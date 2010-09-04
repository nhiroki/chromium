// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome_frame/chrome_launcher_utils.h"

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/win_util.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_switches.h"
#include "chrome_frame/chrome_frame_automation.h"

namespace chrome_launcher {

const wchar_t kLauncherExeBaseName[] = L"chrome_launcher.exe";

CommandLine* CreateLaunchCommandLine() {
  // Shortcut for OS versions that don't need the integrity broker.
  if (win_util::GetWinVersion() < win_util::WINVERSION_VISTA) {
    return new CommandLine(GetChromeExecutablePath());
  }

  // The launcher EXE will be in the same directory as the Chrome Frame DLL,
  // so create a full path to it based on this assumption.  Since our unit
  // tests also use this function, and live in the directory above, we test
  // existence of the file and try the path that includes the /servers/
  // directory if needed.
  FilePath module_path;
  if (PathService::Get(base::FILE_MODULE, &module_path)) {
    FilePath current_dir = module_path.DirName();
    FilePath same_dir_path = current_dir.Append(kLauncherExeBaseName);
    if (file_util::PathExists(same_dir_path)) {
      return new CommandLine(same_dir_path);
    } else {
      FilePath servers_path =
          current_dir.Append(L"servers").Append(kLauncherExeBaseName);
      DCHECK(file_util::PathExists(servers_path)) <<
          "What module is this? It's not in 'servers' or main output dir.";
      return new CommandLine(servers_path);
    }
  } else {
    NOTREACHED();
    return NULL;
  }
}

FilePath GetChromeExecutablePath() {
  FilePath cur_path;
  PathService::Get(base::DIR_MODULE, &cur_path);
  cur_path = cur_path.Append(chrome::kBrowserProcessExecutableName);

  // The installation model for Chrome places the DLLs in a versioned
  // sub-folder one down from the Chrome executable. If we fail to find
  // chrome.exe in the current path, try looking one up and launching that
  // instead.
  if (!file_util::PathExists(cur_path)) {
    PathService::Get(base::DIR_MODULE, &cur_path);
    cur_path = cur_path.DirName().Append(chrome::kBrowserProcessExecutableName);
  }

  return cur_path;
}

}  // namespace chrome_launcher
