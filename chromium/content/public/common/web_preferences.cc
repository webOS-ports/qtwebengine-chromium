// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/web_preferences.h"

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "third_party/WebKit/public/web/WebSettings.h"

using blink::WebSettings;

namespace content {

// "Zyyy" is the ISO 15924 script code for undetermined script aka Common.
const char kCommonScript[] = "Zyyy";

#define STATIC_ASSERT_ENUM(a, b)                            \
  static_assert(static_cast<int>(a) == static_cast<int>(b), \
                "mismatching enums: " #a)

STATIC_ASSERT_ENUM(EDITING_BEHAVIOR_MAC, WebSettings::EditingBehaviorMac);
STATIC_ASSERT_ENUM(EDITING_BEHAVIOR_WIN, WebSettings::EditingBehaviorWin);
STATIC_ASSERT_ENUM(EDITING_BEHAVIOR_UNIX, WebSettings::EditingBehaviorUnix);
STATIC_ASSERT_ENUM(EDITING_BEHAVIOR_ANDROID,
                   WebSettings::EditingBehaviorAndroid);

STATIC_ASSERT_ENUM(V8_CACHE_OPTIONS_DEFAULT,
                   WebSettings::V8CacheOptionsDefault);
STATIC_ASSERT_ENUM(V8_CACHE_OPTIONS_NONE, WebSettings::V8CacheOptionsNone);
STATIC_ASSERT_ENUM(V8_CACHE_OPTIONS_PARSE, WebSettings::V8CacheOptionsParse);
STATIC_ASSERT_ENUM(V8_CACHE_OPTIONS_CODE, WebSettings::V8CacheOptionsCode);
STATIC_ASSERT_ENUM(V8_CACHE_OPTIONS_LAST, WebSettings::V8CacheOptionsCode);

STATIC_ASSERT_ENUM(ProgressBarCompletion::LOAD_EVENT,
                   WebSettings::ProgressBarCompletion::LoadEvent);
STATIC_ASSERT_ENUM(ProgressBarCompletion::RESOURCES_BEFORE_DCL,
                   WebSettings::ProgressBarCompletion::ResourcesBeforeDCL);
STATIC_ASSERT_ENUM(ProgressBarCompletion::DOM_CONTENT_LOADED,
                   WebSettings::ProgressBarCompletion::DOMContentLoaded);
STATIC_ASSERT_ENUM(
    ProgressBarCompletion::RESOURCES_BEFORE_DCL_AND_SAME_ORIGIN_IFRAMES,
    WebSettings::ProgressBarCompletion::ResourcesBeforeDCLAndSameOriginIFrames);

STATIC_ASSERT_ENUM(IMAGE_ANIMATION_POLICY_ALLOWED,
                   WebSettings::ImageAnimationPolicyAllowed);
STATIC_ASSERT_ENUM(IMAGE_ANIMATION_POLICY_ANIMATION_ONCE,
                   WebSettings::ImageAnimationPolicyAnimateOnce);
STATIC_ASSERT_ENUM(IMAGE_ANIMATION_POLICY_NO_ANIMATION,
                   WebSettings::ImageAnimationPolicyNoAnimation);

STATIC_ASSERT_ENUM(ui::POINTER_TYPE_NONE, blink::PointerTypeNone);
STATIC_ASSERT_ENUM(ui::POINTER_TYPE_COARSE, blink::PointerTypeCoarse);
STATIC_ASSERT_ENUM(ui::POINTER_TYPE_FINE, blink::PointerTypeFine);

STATIC_ASSERT_ENUM(ui::HOVER_TYPE_NONE, blink::HoverTypeNone);
STATIC_ASSERT_ENUM(ui::HOVER_TYPE_ON_DEMAND, blink::HoverTypeOnDemand);
STATIC_ASSERT_ENUM(ui::HOVER_TYPE_HOVER, blink::HoverTypeHover);

STATIC_ASSERT_ENUM(ViewportStyle::DEFAULT, blink::WebViewportStyle::Default);
STATIC_ASSERT_ENUM(ViewportStyle::MOBILE, blink::WebViewportStyle::Mobile);
STATIC_ASSERT_ENUM(ViewportStyle::TELEVISION,
                   blink::WebViewportStyle::Television);

WebPreferences::WebPreferences()
    : default_font_size(16),
      default_fixed_font_size(13),
      minimum_font_size(0),
      minimum_logical_font_size(6),
      default_encoding("ISO-8859-1"),
#if defined(OS_WIN)
      context_menu_on_mouse_up(true),
#else
      context_menu_on_mouse_up(false),
#endif
      javascript_enabled(true),
      web_security_enabled(true),
      javascript_can_open_windows_automatically(true),
      loads_images_automatically(true),
      images_enabled(true),
      plugins_enabled(true),
      dom_paste_enabled(false),  // enables execCommand("paste")
      shrinks_standalone_images_to_fit(true),
      uses_universal_detector(false),  // Disabled: page cycler regression
      text_areas_are_resizable(true),
      allow_scripts_to_close_windows(false),
      remote_fonts_enabled(true),
      javascript_can_access_clipboard(false),
      xslt_enabled(true),
      xss_auditor_enabled(true),
      dns_prefetching_enabled(true),
      data_saver_enabled(false),
      local_storage_enabled(false),
      databases_enabled(false),
      application_cache_enabled(false),
      tabs_to_links(true),
      caret_browsing_enabled(false),
      hyperlink_auditing_enabled(true),
      allow_universal_access_from_file_urls(false),
      allow_file_access_from_file_urls(false),
      experimental_webgl_enabled(false),
      pepper_3d_enabled(false),
      flash_3d_enabled(true),
      flash_stage3d_enabled(false),
      flash_stage3d_baseline_enabled(false),
      privileged_webgl_extensions_enabled(false),
      webgl_errors_to_console_enabled(true),
      mock_scrollbars_enabled(false),
      unified_textchecker_enabled(false),
      accelerated_2d_canvas_enabled(false),
      minimum_accelerated_2d_canvas_size(257 * 256),
      disable_2d_canvas_copy_on_write(false),
      antialiased_2d_canvas_disabled(false),
      antialiased_clips_2d_canvas_enabled(false),
      accelerated_2d_canvas_msaa_sample_count(0),
      accelerated_filters_enabled(false),
      deferred_filters_enabled(false),
      container_culling_enabled(false),
      allow_displaying_insecure_content(true),
      allow_running_insecure_content(false),
      disable_reading_from_canvas(false),
      strict_mixed_content_checking(false),
      strict_powerful_feature_restrictions(false),
      allow_geolocation_on_insecure_origins(false),
      strictly_block_blockable_mixed_content(false),
      block_mixed_plugin_content(false),
      password_echo_enabled(false),
      should_print_backgrounds(false),
      should_clear_document_background(true),
      enable_scroll_animator(false),
      enable_error_page(true),
      touch_enabled(false),
      device_supports_touch(false),
      device_supports_mouse(true),
      touch_adjustment_enabled(true),
      pointer_events_max_touch_points(0),
      available_pointer_types(0),
      primary_pointer_type(ui::POINTER_TYPE_NONE),
      available_hover_types(0),
      primary_hover_type(ui::HOVER_TYPE_NONE),
      sync_xhr_in_documents_enabled(true),
      image_color_profiles_enabled(false),
      should_respect_image_orientation(false),
      number_of_cpu_cores(1),
#if defined(OS_MACOSX)
      editing_behavior(EDITING_BEHAVIOR_MAC),
#elif defined(OS_WIN)
      editing_behavior(EDITING_BEHAVIOR_WIN),
#elif defined(OS_ANDROID)
      editing_behavior(EDITING_BEHAVIOR_ANDROID),
#elif defined(OS_POSIX)
      editing_behavior(EDITING_BEHAVIOR_UNIX),
#else
      editing_behavior(EDITING_BEHAVIOR_MAC),
#endif
      supports_multiple_windows(true),
      viewport_enabled(false),
#if defined(OS_ANDROID)
      viewport_meta_enabled(true),
      shrinks_viewport_contents_to_fit(true),
      viewport_style(ViewportStyle::MOBILE),
      always_show_context_menu_on_touch(false),
#else
      viewport_meta_enabled(false),
      shrinks_viewport_contents_to_fit(false),
      viewport_style(ViewportStyle::DEFAULT),
      always_show_context_menu_on_touch(true),
#endif
      main_frame_resizes_are_orientation_changes(false),
      initialize_at_minimum_page_scale(true),
#if defined(OS_MACOSX)
      smart_insert_delete_enabled(true),
#else
      smart_insert_delete_enabled(false),
#endif
      spatial_navigation_enabled(false),
      pinch_overlay_scrollbar_thickness(0),
      use_solid_color_scrollbars(false),
      navigate_on_drag_drop(true),
      v8_cache_options(V8_CACHE_OPTIONS_DEFAULT),
      inert_visual_viewport(false),
      record_whole_document(false),
      cookie_enabled(true),
      pepper_accelerated_video_decode_enabled(false),
      animation_policy(IMAGE_ANIMATION_POLICY_ALLOWED),
      user_gesture_required_for_presentation(true),
      text_track_margin_percentage(0.0f),
#if defined(OS_ANDROID)
      text_autosizing_enabled(true),
      font_scale_factor(1.0f),
      device_scale_adjustment(1.0f),
      force_enable_zoom(false),
      fullscreen_supported(true),
      double_tap_to_zoom_enabled(true),
      user_gesture_required_for_media_playback(true),
      support_deprecated_target_density_dpi(false),
      use_legacy_background_size_shorthand_behavior(false),
      wide_viewport_quirk(false),
      use_wide_viewport(true),
      force_zero_layout_height(false),
      viewport_meta_layout_size_quirk(false),
      viewport_meta_merge_content_quirk(false),
      viewport_meta_non_user_scalable_quirk(false),
      viewport_meta_zero_values_quirk(false),
      clobber_user_agent_initial_scale_quirk(false),
      ignore_main_frame_overflow_hidden_quirk(false),
      report_screen_size_in_physical_pixels_quirk(false),
      resue_global_for_unowned_main_frame(false),
      autoplay_muted_videos_enabled(false),
      progress_bar_completion(ProgressBarCompletion::LOAD_EVENT),
#elif defined(TOOLKIT_QT)
      fullscreen_supported(false),
#endif
#if defined(OS_ANDROID)
      default_minimum_page_scale_factor(0.25f),
      default_maximum_page_scale_factor(5.f)
#elif defined(OS_MACOSX)
      default_minimum_page_scale_factor(1.f),
      default_maximum_page_scale_factor(3.f)
#else
      default_minimum_page_scale_factor(1.f),
      default_maximum_page_scale_factor(4.f)
#endif
      ,
      luneosPriviledged(false),
      palmServiceBridgeEnabled(false),
      luneosAppIdentifier("")
{
  standard_font_family_map[kCommonScript] =
      base::ASCIIToUTF16("Times New Roman");
  fixed_font_family_map[kCommonScript] = base::ASCIIToUTF16("Courier New");
  serif_font_family_map[kCommonScript] = base::ASCIIToUTF16("Times New Roman");
  sans_serif_font_family_map[kCommonScript] = base::ASCIIToUTF16("Arial");
  cursive_font_family_map[kCommonScript] = base::ASCIIToUTF16("Script");
  fantasy_font_family_map[kCommonScript] = base::ASCIIToUTF16("Impact");
  pictograph_font_family_map[kCommonScript] =
      base::ASCIIToUTF16("Times New Roman");
}

WebPreferences::WebPreferences(const WebPreferences& other) = default;

WebPreferences::~WebPreferences() {
}

}  // namespace content
