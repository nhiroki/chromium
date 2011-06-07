#
# Copyright (C) 2009 Google Inc. All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
# 
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

{
  # The following defines turn webkit features on and off.
  'variables': {
    'variables': {
      # We have to nest variables inside variables so that they can be
      # overridden through GYP_DEFINES.
      'variables': {
        'enable_register_protocol_handler%' : 1,
        'enable_svg%': 1,
        'enable_touch_events%': 1,
        'enable_touch_icon_loading%' : 0,
        'use_accelerated_compositing%': 1,
        'use_skia_gpu%': 0,
        'use_threaded_compositing%': 0,
      },

      # We have to nest variables inside variables as a hack for variables
      # override.
      # FIXME: We probably won't need to do that once we finish killing
      # feature_override.gypi. We should be able to remove a whole level of
      # nesting at that time.

      # WARNING: build/features_override.gypi which is included in a full
      # chromium build, overrides this list with its own values. See
      # features_override.gypi inline documentation for more details.
      # FIXME: Remove features_override.gypi.
      'feature_defines': [
        'ENABLE_3D_RENDERING=1',
        'ENABLE_BLOB=1',
        'ENABLE_BLOB_SLICE=1',
        'ENABLE_CHANNEL_MESSAGING=1',
        'ENABLE_CLIENT_BASED_GEOLOCATION=1',
        'ENABLE_DASHBOARD_SUPPORT=0',
        'ENABLE_DATABASE=1',
        'ENABLE_DATA_TRANSFER_ITEMS=1',
        'ENABLE_DETAILS=1',
        'ENABLE_DEVICE_ORIENTATION=1',
        'ENABLE_DIRECTORY_UPLOAD=1',
        'ENABLE_DOM_STORAGE=1',
        'ENABLE_EVENTSOURCE=1',
        'ENABLE_FILE_SYSTEM=1',
        'ENABLE_FILTERS=1',
        'ENABLE_FULLSCREEN_API=1',
        'ENABLE_GEOLOCATION=1',
        'ENABLE_GESTURE_RECOGNIZER=1',
        'ENABLE_ICONDATABASE=0',
        'ENABLE_INDEXED_DATABASE=1',
        'ENABLE_INPUT_COLOR=1',
        'ENABLE_INPUT_SPEECH=1',
        'ENABLE_JAVASCRIPT_DEBUGGER=1',
        'ENABLE_JAVASCRIPT_I18N_API=1',
        'ENABLE_JSC_MULTIPLE_THREADS=0',
        'ENABLE_LEVELDB=1',
        'ENABLE_LINK_PREFETCH=1',
        'ENABLE_MATHML=0',
        'ENABLE_MEDIA_STATISTICS=1',
        'ENABLE_MEDIA_STREAM=1',
        'ENABLE_METER_TAG=1',
        'ENABLE_MHTML=1',
        'ENABLE_NOTIFICATIONS=1',
        'ENABLE_OFFLINE_WEB_APPLICATIONS=1',
        'ENABLE_OPENTYPE_SANITIZER=1',
        'ENABLE_ORIENTATION_EVENTS=0',
        'ENABLE_PAGE_VISIBILITY_API=1',
        'ENABLE_PROGRESS_TAG=1',
        'ENABLE_QUOTA=1',
        'ENABLE_REQUEST_ANIMATION_FRAME=1',
        'ENABLE_SHARED_WORKERS=1',
        'ENABLE_SKIA_GPU=<(use_skia_gpu)',
        'ENABLE_SVG=<(enable_svg)',
        'ENABLE_SVG_ANIMATION=<(enable_svg)',
        'ENABLE_SVG_AS_IMAGE=<(enable_svg)',
        'ENABLE_SVG_FONTS=<(enable_svg)',
        'ENABLE_SVG_FOREIGN_OBJECT=<(enable_svg)',
        'ENABLE_SVG_USE=<(enable_svg)',
        'ENABLE_TOUCH_EVENTS=<(enable_touch_events)',
        'ENABLE_TOUCH_ICON_LOADING=<(enable_touch_icon_loading)',
        'ENABLE_V8_SCRIPT_DEBUG_SERVER=1',
        'ENABLE_VIDEO=1',
        'ENABLE_WEBGL=1',
        'ENABLE_WEB_SOCKETS=1',
        'ENABLE_WEB_TIMING=1',
        'ENABLE_WORKERS=1',
        'ENABLE_XHR_RESPONSE_BLOB=1',
        'ENABLE_XPATH=1',
        'ENABLE_XSLT=1',
        'WTF_USE_WEBKIT_IMAGE_DECODERS=1',
        'WTF_USE_WEBP=1',
      ],

      'feature_defines%': '<(feature_defines)',

      'enable_register_protocol_handler%': '<(enable_register_protocol_handler)',
      'enable_svg%': '<(enable_svg)',
      'enable_touch_events%': '<(enable_touch_events)',
      'enable_touch_icon_loading%': '<(enable_touch_icon_loading)',
      'use_accelerated_compositing%': '<(use_accelerated_compositing)',
      'use_skia_gpu%': '<(use_skia_gpu)',
      'use_threaded_compositing%': '<(use_threaded_compositing)',

      'conditions': [
        ['use_accelerated_compositing==1', {
          'feature_defines': [
            'WTF_USE_ACCELERATED_COMPOSITING=1',
            'ENABLE_3D_RENDERING=1',
          ],
        }],
        ['use_accelerated_compositing==1 and OS!="mac"', {
          'feature_defines': [
            'ENABLE_ACCELERATED_2D_CANVAS=1',
          ],
        }],
        ['use_accelerated_compositing==1 and use_threaded_compositing==1', {
          'feature_defines': [
            'WTF_USE_THREADED_COMPOSITING=1',
          ],
        }],
        ['touchui==1', {
          'enable_touch_icon_loading': 1,
        }],
        # FIXME: For the moment Windows is only enabled for Google-branded
        # build, since the FFmpeg DLLs need to be re-built for chromium.
        ['OS=="mac" or OS=="linux" or (OS=="win" and branding=="Chrome")', {
          'feature_defines': [
            'ENABLE_WEB_AUDIO=1',
          ],
        }],
        # Mac OS X uses Accelerate.framework FFT by default instead of FFmpeg.
        ['OS!="mac"', {
          'feature_defines': [
            'WTF_USE_WEBAUDIO_FFMPEG=1',
          ],
        }],
        ['enable_register_protocol_handler==1', {
          'feature_defines': [
            'ENABLE_REGISTER_PROTOCOL_HANDLER=1',
          ],
        }],
      ],
    },

    'feature_defines%': '<(feature_defines)',

    'enable_register_protocol_handler%': '<(enable_register_protocol_handler)',
    'enable_svg%': '<(enable_svg)',
    'enable_touch_events%': '<(enable_touch_events)',
    'enable_touch_icon_loading%': '<(enable_touch_icon_loading)',
    'use_accelerated_compositing%': '<(use_accelerated_compositing)',
    'use_skia_gpu%': '<(use_skia_gpu)',
    'use_threaded_compositing%': '<(use_threaded_compositing)',
  },
}
