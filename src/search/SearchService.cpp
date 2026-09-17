#include "search/SearchService.hpp"

#include <charconv>
#include <cerrno>
#include <csignal>
#include <fstream>
#include <system_error>

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

namespace puka {

namespace {

std::string TruncatePreview(std::string_view text, size_t max_len = 300) {
  if (text.size() <= max_len) return std::string(text);
  return std::string(text.substr(0, max_len)) + "\xE2\x80\xA6";  // "…"
}

using LineParser = std::optional<SearchHit> (*)(std::string_view);

struct SubprocessOutcome {
  bool tool_found = false;  // false => execvp couldn't find the binary at all
  int exit_code = -1;
  bool killed_for_cap = false;
  std::vector<SearchHit> hits;
  bool truncated = false;
};

// Runs `argv[0]` via fork()+execvp() (never a shell -- the query is arbitrary
// user text and must never be shell-interpolated), reads its stdout line by
// line through `parse_line`, and stops early (SIGTERM) once `max_results`
// hits accumulate. Whether the binary was found at all is signaled back via
// a second, CLOEXEC-on-both-ends pipe: on successful execvp the kernel
// closes its write end automatically as part of exec(), so the parent's
// read() returns 0 almost immediately; on failure the child writes errno
// into it before _exit(127). This is race-free and distinguishes "not
// installed" from "ran and exited nonzero" without any shell involved.
SubprocessOutcome RunSubprocess(const std::vector<std::string>& argv, size_t max_results,
                                 LineParser parse_line) {
  SubprocessOutcome outcome;

  int out_fds[2];
  if (::pipe(out_fds) != 0) return outcome;
  int err_fds[2];
  if (::pipe(err_fds) != 0) {
    ::close(out_fds[0]);
    ::close(out_fds[1]);
    return outcome;
  }
  ::fcntl(err_fds[0], F_SETFD, FD_CLOEXEC);
  ::fcntl(err_fds[1], F_SETFD, FD_CLOEXEC);

  pid_t pid = ::fork();
  if (pid < 0) {
    ::close(out_fds[0]);
    ::close(out_fds[1]);
    ::close(err_fds[0]);
    ::close(err_fds[1]);
    return outcome;
  }

  if (pid == 0) {
    // --- child ---
    ::close(err_fds[0]);
    ::close(out_fds[0]);
    ::dup2(out_fds[1], STDOUT_FILENO);
    ::close(out_fds[1]);
    int devnull = ::open("/dev/null", O_WRONLY);
    if (devnull >= 0) {
      ::dup2(devnull, STDERR_FILENO);
      ::close(devnull);
    }

    std::vector<char*> c_argv;
    c_argv.reserve(argv.size() + 1);
    for (const auto& s : argv) c_argv.push_back(const_cast<char*>(s.c_str()));
    c_argv.push_back(nullptr);

    ::execvp(c_argv[0], c_argv.data());
    int err = errno;  // execvp only returns on failure
    ssize_t written = ::write(err_fds[1], &err, sizeof(err));
    (void)written;
    ::_exit(127);
  }

  // --- parent ---
  ::close(err_fds[1]);
  ::close(out_fds[1]);

  int exec_errno = 0;
  ssize_t n = ::read(err_fds[0], &exec_errno, sizeof(exec_errno));
  ::close(err_fds[0]);

  if (n > 0) {
    // execvp failed in the child (classically ENOENT: tool not installed).
    ::close(out_fds[0]);
    int status = 0;
    ::waitpid(pid, &status, 0);
    return outcome;  // tool_found stays false
  }

  outcome.tool_found = true;
  std::string buffer;
  char chunk[4096];
  bool capped = false;
  while (!capped) {
    ssize_t r = ::read(out_fds[0], chunk, sizeof(chunk));
    if (r < 0) {
      if (errno == EINTR) continue;
      break;
    }
    if (r == 0) break;  // EOF
    buffer.append(chunk, static_cast<size_t>(r));

    size_t start = 0;
    size_t nl;
    std::string_view buf_view(buffer);
    while (!capped && (nl = buf_view.find('\n', start)) != std::string_view::npos) {
      if (auto hit = parse_line(buf_view.substr(start, nl - start))) {
        outcome.hits.push_back(std::move(*hit));
        if (outcome.hits.size() >= max_results) {
          outcome.truncated = true;
          capped = true;
        }
      }
      start = nl + 1;
    }
    buffer.erase(0, start);
  }

  if (capped) {
    // Bound worst-case latency on huge/pathological repos rather than
    // letting the child finish walking the whole tree.
    ::kill(pid, SIGTERM);
    outcome.killed_for_cap = true;
  } else if (!buffer.empty()) {
    // Trailing line with no final '\n'.
    if (auto hit = parse_line(buffer)) outcome.hits.push_back(std::move(*hit));
  }

  // Close our read end now. If capped and the child is still writing, its
  // next write() gets EPIPE/SIGPIPE (terminating it) instead of blocking --
  // so waitpid below cannot hang regardless of unread buffered output.
  ::close(out_fds[0]);

  int status = 0;
  ::waitpid(pid, &status, 0);
  if (outcome.killed_for_cap) {
    outcome.exit_code = 0;  // a capped search is still a success
  } else if (WIFEXITED(status)) {
    outcome.exit_code = WEXITSTATUS(status);
  } else {
    outcome.exit_code = -1;  // killed by something else -> reported as an error
  }
  return outcome;
}

std::string ToolName(SearchTool tool) {
  switch (tool) {
    case SearchTool::Ripgrep: return "rg";
    case SearchTool::Grep: return "grep";
    case SearchTool::PlainScan: return "built-in scan";
  }
  return "";
}

// Falling through to the next tool happens ONLY when the tool wasn't found
// at all (tool_found == false, handled by the caller before reaching here).
// Exit codes 0 and 1 both mean "the tool ran" (match/no-match -- decided by
// the parsed hit count, never the exit code, since e.g. `rg --vimgrep` can
// print a non-conforming binary-file summary line with exit code 0). Any
// other exit code is a genuine tool error, reported via `note` without
// cascading -- a real error on this machine shouldn't be silently masked as
// "0 matches".
SearchResult BuildResult(SearchTool tool, SubprocessOutcome outcome, std::string fallback_note) {
  SearchResult r;
  r.tool_used = tool;
  r.hits = std::move(outcome.hits);
  r.truncated = outcome.truncated;
  if (outcome.exit_code == 0 || outcome.exit_code == 1) {
    r.note = std::move(fallback_note);
  } else {
    r.note = ToolName(tool) + " exited with an error (code " + std::to_string(outcome.exit_code) + ")";
  }
  return r;
}

SearchResult RunPlainScan(const std::string& query, const std::filesystem::path& root,
                          size_t max_results) {
  SearchResult result;
  result.tool_used = SearchTool::PlainScan;
  result.note = "ripgrep and grep not found -- used a built-in scan";

  std::error_code ec;
  auto it = std::filesystem::recursive_directory_iterator(
      root, std::filesystem::directory_options::skip_permission_denied, ec);
  const auto end = std::filesystem::recursive_directory_iterator();
  for (; !ec && it != end; it.increment(ec)) {
    const auto entry = *it;
    if (entry.is_directory()) {
      if (entry.path().filename() == ".git") it.disable_recursion_pending();
      continue;
    }
    std::error_code file_ec;
    if (!entry.is_regular_file(file_ec) || file_ec) continue;

    std::ifstream in(entry.path(), std::ios::binary);
    if (!in) continue;
    std::string file_line;
    size_t line_no = 0;
    while (std::getline(in, file_line)) {
      ++line_no;
      size_t pos = file_line.find(query);
      if (pos == std::string::npos) continue;
      SearchHit hit;
      hit.file = entry.path();
      hit.line = line_no;
      hit.column = pos + 1;
      hit.preview = TruncatePreview(file_line);
      result.hits.push_back(std::move(hit));
      if (result.hits.size() >= max_results) {
        result.truncated = true;
        return result;
      }
    }
  }
  return result;
}

}  // namespace

std::optional<SearchHit> ParseRipgrepVimgrepLine(std::string_view line) {
  size_t c1 = line.find(':');
  if (c1 == std::string_view::npos) return std::nullopt;
  size_t c2 = line.find(':', c1 + 1);
  if (c2 == std::string_view::npos) return std::nullopt;
  size_t c3 = line.find(':', c2 + 1);
  if (c3 == std::string_view::npos) return std::nullopt;

  std::string_view path_sv = line.substr(0, c1);
  std::string_view line_sv = line.substr(c1 + 1, c2 - c1 - 1);
  std::string_view col_sv = line.substr(c2 + 1, c3 - c2 - 1);
  std::string_view text_sv = line.substr(c3 + 1);
  if (path_sv.empty()) return std::nullopt;

  size_t line_no = 0, col_no = 0;
  auto r1 = std::from_chars(line_sv.data(), line_sv.data() + line_sv.size(), line_no);
  if (r1.ec != std::errc() || r1.ptr != line_sv.data() + line_sv.size()) return std::nullopt;
  auto r2 = std::from_chars(col_sv.data(), col_sv.data() + col_sv.size(), col_no);
  if (r2.ec != std::errc() || r2.ptr != col_sv.data() + col_sv.size()) return std::nullopt;

  SearchHit hit;
  hit.file = std::filesystem::path(path_sv);
  hit.line = line_no;
  hit.column = col_no;
  hit.preview = TruncatePreview(text_sv);
  return hit;
}

std::optional<SearchHit> ParseGrepLine(std::string_view line) {
  size_t c1 = line.find(':');
  if (c1 == std::string_view::npos) return std::nullopt;
  size_t c2 = line.find(':', c1 + 1);
  if (c2 == std::string_view::npos) return std::nullopt;

  std::string_view path_sv = line.substr(0, c1);
  std::string_view line_sv = line.substr(c1 + 1, c2 - c1 - 1);
  std::string_view text_sv = line.substr(c2 + 1);
  if (path_sv.empty()) return std::nullopt;

  size_t line_no = 0;
  auto r = std::from_chars(line_sv.data(), line_sv.data() + line_sv.size(), line_no);
  if (r.ec != std::errc() || r.ptr != line_sv.data() + line_sv.size()) return std::nullopt;

  SearchHit hit;
  hit.file = std::filesystem::path(path_sv);
  hit.line = line_no;
  hit.column = 1;
  hit.preview = TruncatePreview(text_sv);
  return hit;
}

SearchResult CapResults(std::vector<SearchHit> hits, size_t max_results) {
  SearchResult r;
  r.truncated = hits.size() > max_results;
  if (r.truncated) hits.resize(max_results);
  r.hits = std::move(hits);
  return r;
}

SearchResult RunSearch(const std::string& query, const std::filesystem::path& root,
                       size_t max_results) {
  if (query.empty()) return {};

  std::error_code ec;
  auto abs_root = std::filesystem::absolute(root, ec);
  if (ec) abs_root = root;

  auto rg_out = RunSubprocess(
      {"rg", "--vimgrep", "--color=never", "--fixed-strings", "--", query, abs_root.string()},
      max_results, &ParseRipgrepVimgrepLine);
  if (rg_out.tool_found) return BuildResult(SearchTool::Ripgrep, std::move(rg_out), "");

  auto grep_out = RunSubprocess(
      {"grep", "-r", "-n", "--fixed-strings", "--exclude-dir=.git", "--exclude-dir=build", "--",
       query, abs_root.string()},
      max_results, &ParseGrepLine);
  if (grep_out.tool_found)
    return BuildResult(SearchTool::Grep, std::move(grep_out), "ripgrep not found -- used grep");

  return RunPlainScan(query, abs_root, max_results);
}

}  // namespace puka
