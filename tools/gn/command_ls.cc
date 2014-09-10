// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <set>

#include "base/command_line.h"
#include "tools/gn/commands.h"
#include "tools/gn/label_pattern.h"
#include "tools/gn/setup.h"
#include "tools/gn/standard_out.h"
#include "tools/gn/target.h"

namespace commands {

const char kLs[] = "ls";
const char kLs_HelpShort[] =
    "ls: List matching targets.";
const char kLs_Help[] =
    "gn ls <build dir> [<label_pattern>] [--out] [--all-toolchains]\n"
    "\n"
    "  Lists all targets matching the given pattern for the given builn\n"
    "  directory. By default, only targets in the default toolchain will\n"
    "  be matched unless a toolchain is explicitly supplied.\n"
    "\n"
    "  If the label pattern is unspecified, list all targets. The label\n"
    "  pattern is not a general regular expression (see\n"
    "  \"gn help label_pattern\"). If you need more complex expressions,\n"
    "  pipe the result through grep.\n"
    "\n"
    "  --out\n"
    "      Lists the results as the files generated by the matching targets.\n"
    "      These files will be relative to the build directory such that\n"
    "      they can be specified on Ninja's command line as a file to build.\n"
    "\n"
    "  --all-toolchains\n"
    "      Matches all toolchains. If the label pattern does not specify an\n"
    "      explicit toolchain, labels from all toolchains will be matched.\n"
    "\n"
    "Examples\n"
    "\n"
    "  gn ls out/Debug\n"
    "      Lists all targets in the default toolchain.\n"
    "\n"
    "  gn ls out/Debug \"//base/*\"\n"
    "      Lists all targets in the directory base and all subdirectories.\n"
    "\n"
    "  gn ls out/Debug \"//base:*\"\n"
    "      Lists all targets defined in //base/BUILD.gn.\n"
    "\n"
    "  gn ls out/Debug //base --out\n"
    "      Lists the build output file for //base:base\n"
    "\n"
    "  gn ls out/Debug \"//base/*\" --out | xargs ninja -C out/Debug\n"
    "      Builds all targets in //base and all subdirectories.\n"
    "\n"
    "  gn ls out/Debug //base --all-toolchains\n"
    "      Lists all variants of the target //base:base (it may be referenced\n"
    "      in multiple toolchains).\n";

int RunLs(const std::vector<std::string>& args) {
  if (args.size() != 1 && args.size() != 2) {
    Err(Location(), "You're holding it wrong.",
        "Usage: \"gn ls <build dir> [<label_pattern>]\"").PrintToStdout();
    return 1;
  }

  Setup* setup = new Setup;
  if (!setup->DoSetup(args[0], false) || !setup->Run())
    return 1;

  const CommandLine* cmdline = CommandLine::ForCurrentProcess();
  bool all_toolchains = cmdline->HasSwitch("all-toolchains");

  // Find matching targets.
  std::vector<const Target*> matches;
  if (args.size() == 2) {
    // Given a pattern, match it.
    if (!ResolveTargetsFromCommandLinePattern(setup, args[1], all_toolchains,
                                              &matches))
      return 1;
  } else if (all_toolchains) {
    // List all resolved targets.
    matches = setup->builder()->GetAllResolvedTargets();
  } else {
    // List all resolved targets in the default toolchain.
    std::vector<const Target*> all_targets =
        setup->builder()->GetAllResolvedTargets();
    for (size_t i = 0; i < all_targets.size(); i++) {
      if (all_targets[i]->settings()->is_default())
        matches.push_back(all_targets[i]);
    }
  }

  if (cmdline->HasSwitch("out")) {
    // List results as build files.
    for (size_t i = 0; i < matches.size(); i++) {
      OutputString(matches[i]->dependency_output_file().value());
      OutputString("\n");
    }
  } else {
    // List results as sorted labels.
    std::vector<Label> sorted_matches;
    for (size_t i = 0; i < matches.size(); i++)
      sorted_matches.push_back(matches[i]->label());
    std::sort(sorted_matches.begin(), sorted_matches.end());

    Label default_tc_label = setup->loader()->default_toolchain_label();
    for (size_t i = 0; i < sorted_matches.size(); i++) {
      // Print toolchain only for ones not in the default toolchain.
      OutputString(sorted_matches[i].GetUserVisibleName(
          sorted_matches[i].GetToolchainLabel() != default_tc_label));
      OutputString("\n");
    }
  }

  return 0;
}

}  // namespace commands
