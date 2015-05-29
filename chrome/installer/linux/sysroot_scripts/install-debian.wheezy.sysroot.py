#!/usr/bin/env python
# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script to install a Debian Wheezy sysroot for making official Google Chrome
# Linux builds.
# The sysroot is needed to make Chrome work for Debian Wheezy.
# This script can be run manually but is more often run as part of gclient
# hooks. When run from hooks this script should be a no-op on non-linux
# platforms.

# The sysroot image could be constructed from scratch based on the current
# state or Debian Wheezy but for consistency we currently use a pre-built root
# image. The image will normally need to be rebuilt every time chrome's build
# dependancies are changed.

import hashlib
import platform
import optparse
import os
import re
import shutil
import subprocess
import sys


SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
URL_PREFIX = 'http://storage.googleapis.com'
URL_PATH = 'chrome-linux-sysroot/toolchain'
REVISION_AMD64 = 'a2d45701cb21244b9514e420950ba6ba687fb655'
REVISION_I386 = 'a2d45701cb21244b9514e420950ba6ba687fb655'
REVISION_ARM = 'a2d45701cb21244b9514e420950ba6ba687fb655'
TARBALL_AMD64 = 'debian_wheezy_amd64_sysroot.tgz'
TARBALL_I386 = 'debian_wheezy_i386_sysroot.tgz'
TARBALL_ARM = 'debian_wheezy_arm_sysroot.tgz'
TARBALL_AMD64_SHA1SUM = '601216c0f980e798e7131635f3dd8171b3dcbcde'
TARBALL_I386_SHA1SUM = '0090e5a4b56ab9ffb5d557da6a520195ab59b446'
TARBALL_ARM_SHA1SUM = '6289593b36616526562a4d85ae9c92b694b8ce7e'
SYSROOT_DIR_AMD64 = 'debian_wheezy_amd64-sysroot'
SYSROOT_DIR_I386 = 'debian_wheezy_i386-sysroot'
SYSROOT_DIR_ARM = 'debian_wheezy_arm-sysroot'

valid_archs = ('arm', 'i386', 'amd64')


def GetSha1(filename):
  sha1 = hashlib.sha1()
  with open(filename, 'rb') as f:
    while True:
      # Read in 1mb chunks, so it doesn't all have to be loaded into memory.
      chunk = f.read(1024*1024)
      if not chunk:
        break
      sha1.update(chunk)
  return sha1.hexdigest()


def DetectArch(gyp_defines):
  # Check for optional target_arch and only install for that architecture.
  # If target_arch is not specified, then only install for the host
  # architecture.
  if 'target_arch=x64' in gyp_defines:
    return 'amd64'
  elif 'target_arch=ia32' in gyp_defines:
    return 'i386'
  elif 'target_arch=arm' in gyp_defines:
    return 'arm'

  # Figure out host arch using build/detect_host_arch.py and
  # set target_arch to host arch
  SRC_DIR = os.path.abspath(
      os.path.join(SCRIPT_DIR, '..', '..', '..', '..'))
  sys.path.append(os.path.join(SRC_DIR, 'build'))
  import detect_host_arch

  detected_host_arch = detect_host_arch.HostArch()
  if detected_host_arch == 'x64':
    return 'amd64'
  elif detected_host_arch == 'ia32':
    return 'i386'
  elif detected_host_arch == 'arm':
    return 'arm'
  else:
    print "Unknown host arch: %s" % detected_host_arch

  return None


def main():
  if options.running_as_hook and not sys.platform.startswith('linux'):
    return 0

  gyp_defines = os.environ.get('GYP_DEFINES', '')

  if options.arch:
    target_arch = options.arch
  else:
    target_arch = DetectArch(gyp_defines)
    if not target_arch:
      print 'Unable to detect host architecture'
      return 1

  if options.running_as_hook and target_arch != 'arm':
    # When run from runhooks, only install the sysroot for an Official Chrome
    # Linux build, except on ARM where we always use a sysroot.
    skip_if_defined = ['branding=Chrome', 'buildtype=Official']
    skip_if_undefined = ['chromeos=1']
    for option in skip_if_defined:
      if option not in gyp_defines:
        return 0
    for option in skip_if_undefined:
      if option in gyp_defines:
        return 0

  # The sysroot directory should match the one specified in build/common.gypi.
  # TODO(thestig) Consider putting this else where to avoid having to recreate
  # it on every build.
  linux_dir = os.path.dirname(SCRIPT_DIR)
  if target_arch == 'amd64':
    sysroot = os.path.join(linux_dir, SYSROOT_DIR_AMD64)
    tarball_filename = TARBALL_AMD64
    tarball_sha1sum = TARBALL_AMD64_SHA1SUM
    revision = REVISION_AMD64
  elif target_arch == 'arm':
    sysroot = os.path.join(linux_dir, SYSROOT_DIR_ARM)
    tarball_filename = TARBALL_ARM
    tarball_sha1sum = TARBALL_ARM_SHA1SUM
    revision = REVISION_ARM
  elif target_arch == 'i386':
    sysroot = os.path.join(linux_dir, SYSROOT_DIR_I386)
    tarball_filename = TARBALL_I386
    tarball_sha1sum = TARBALL_I386_SHA1SUM
    revision = REVISION_I386
  else:
    print 'Unknown architecture: %s' % target_arch
    assert(False)

  url = '%s/%s/%s/%s' % (URL_PREFIX, URL_PATH, revision, tarball_filename)

  stamp = os.path.join(sysroot, '.stamp')
  if os.path.exists(stamp):
    with open(stamp) as s:
      if s.read() == url:
        print 'Debian Wheezy %s root image already up-to-date: %s' % \
            (target_arch, sysroot)
        return 0

  print 'Installing Debian Wheezy %s root image: %s' % (target_arch, sysroot)
  if os.path.isdir(sysroot):
    shutil.rmtree(sysroot)
  os.mkdir(sysroot)
  tarball = os.path.join(sysroot, tarball_filename)
  print 'Downloading %s' % url
  sys.stdout.flush()
  sys.stderr.flush()
  subprocess.check_call(['curl', '--fail', '-L', url, '-o', tarball])
  sha1sum = GetSha1(tarball)
  if sha1sum != tarball_sha1sum:
    print 'Tarball sha1sum is wrong.'
    print 'Expected %s, actual: %s' % (tarball_sha1sum, sha1sum)
    return 1
  subprocess.check_call(['tar', 'xf', tarball, '-C', sysroot])
  os.remove(tarball)

  with open(stamp, 'w') as s:
    s.write(url)
  return 0


if __name__ == '__main__':
  parser = optparse.OptionParser('usage: %prog [OPTIONS]')
  parser.add_option('--running-as-hook', action='store_true',
                    default=False, help='Used when running from gclient hooks.'
                                        ' In this mode the sysroot will only '
                                        'be installed for official Linux '
                                        'builds or ARM Linux builds')
  parser.add_option('--arch', type='choice', choices=valid_archs,
                    help='Sysroot architecture: %s' % ', '.join(valid_archs))
  options, _ = parser.parse_args()
  sys.exit(main())
