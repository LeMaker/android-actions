/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package android.webgl.cts;

import android.webgl.WebGLActivity;
import android.webgl.cts.R;
import android.test.ActivityInstrumentationTestCase2;
import java.io.InputStream;

/**
 * A simple wrapper to load each WebGL conformance test in WebView.
 *
 * This test uses {@link android.test.ActivityInstrumentationTestCase2} to instrument the
 * {@link android.webgl.WebGLActivity}.
 */
public class WebGLTest extends ActivityInstrumentationTestCase2<WebGLActivity> {

    /**
     * A reference to the activity whose shared preferences are being tested.
     */
    private WebGLActivity mActivity;
    private WebGLConformanceSuite mWebGL_1_0_1;

    public WebGLTest() {
        super(WebGLActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        // Start the activity and get a reference to it.
        mActivity = getActivity();
        // Wait for the UI Thread to become idle.
        getInstrumentation().waitForIdleSync();
        mWebGL_1_0_1 = WebGLConformanceSuite.init(mActivity);
    }

    @Override
    protected void tearDown() throws Exception {
        // Scrub the activity so it can be freed. The next time the setUp will create a new activity
        // rather than reusing the old one.
        mActivity = null;
        super.tearDown();
    }

    protected void doTest(String testPage) throws Exception {
        mActivity.navigateToTest(testPage);
    }

    /**
     * The remainder of this file is generated using this command:
     * extract_webgl_tests.py tests 1.0.1
     */
    public void test_conformance_attribs_gl_enable_vertex_attrib_html() throws Exception { doTest("tests/conformance/attribs/gl-enable-vertex-attrib.html"); }
    public void test_conformance_attribs_gl_vertex_attrib_zero_issues_html() throws Exception { doTest("tests/conformance/attribs/gl-vertex-attrib-zero-issues.html"); }
    public void test_conformance_attribs_gl_vertex_attrib_html() throws Exception { doTest("tests/conformance/attribs/gl-vertex-attrib.html"); }
    public void test_conformance_attribs_gl_vertexattribpointer_offsets_html() throws Exception { doTest("tests/conformance/attribs/gl-vertexattribpointer-offsets.html"); }
    public void test_conformance_attribs_gl_vertexattribpointer_html() throws Exception { doTest("tests/conformance/attribs/gl-vertexattribpointer.html"); }
    public void test_conformance_buffers_buffer_bind_test_html() throws Exception { doTest("tests/conformance/buffers/buffer-bind-test.html"); }
    public void test_conformance_buffers_buffer_data_array_buffer_html() throws Exception { doTest("tests/conformance/buffers/buffer-data-array-buffer.html"); }
    public void test_conformance_buffers_index_validation_copies_indices_html() throws Exception { doTest("tests/conformance/buffers/index-validation-copies-indices.html"); }
    public void test_conformance_buffers_index_validation_crash_with_buffer_sub_data_html() throws Exception { doTest("tests/conformance/buffers/index-validation-crash-with-buffer-sub-data.html"); }
    public void test_conformance_buffers_index_validation_verifies_too_many_indices_html() throws Exception { doTest("tests/conformance/buffers/index-validation-verifies-too-many-indices.html"); }
    public void test_conformance_buffers_index_validation_with_resized_buffer_html() throws Exception { doTest("tests/conformance/buffers/index-validation-with-resized-buffer.html"); }
    public void test_conformance_buffers_index_validation_html() throws Exception { doTest("tests/conformance/buffers/index-validation.html"); }
    public void test_conformance_canvas_buffer_offscreen_test_html() throws Exception { doTest("tests/conformance/canvas/buffer-offscreen-test.html"); }
    public void test_conformance_canvas_buffer_preserve_test_html() throws Exception { doTest("tests/conformance/canvas/buffer-preserve-test.html"); }
    public void test_conformance_canvas_canvas_test_html() throws Exception { doTest("tests/conformance/canvas/canvas-test.html"); }
    public void test_conformance_canvas_canvas_zero_size_html() throws Exception { doTest("tests/conformance/canvas/canvas-zero-size.html"); }
    public void test_conformance_canvas_drawingbuffer_static_canvas_test_html() throws Exception { doTest("tests/conformance/canvas/drawingbuffer-static-canvas-test.html"); }
    public void test_conformance_canvas_drawingbuffer_test_html() throws Exception { doTest("tests/conformance/canvas/drawingbuffer-test.html"); }
    public void test_conformance_canvas_viewport_unchanged_upon_resize_html() throws Exception { doTest("tests/conformance/canvas/viewport-unchanged-upon-resize.html"); }
    public void test_conformance_context_constants_and_properties_html() throws Exception { doTest("tests/conformance/context/constants-and-properties.html"); }
    public void test_conformance_context_context_attributes_alpha_depth_stencil_antialias_html() throws Exception { doTest("tests/conformance/context/context-attributes-alpha-depth-stencil-antialias.html"); }
    public void test_conformance_context_context_lost_restored_html() throws Exception { doTest("tests/conformance/context/context-lost-restored.html"); }
    public void test_conformance_context_context_lost_html() throws Exception { doTest("tests/conformance/context/context-lost.html"); }
    public void test_conformance_context_context_type_test_html() throws Exception { doTest("tests/conformance/context/context-type-test.html"); }
    public void test_conformance_context_incorrect_context_object_behaviour_html() throws Exception { doTest("tests/conformance/context/incorrect-context-object-behaviour.html"); }
    public void test_conformance_context_methods_html() throws Exception { doTest("tests/conformance/context/methods.html"); }
    public void test_conformance_context_premultiplyalpha_test_html() throws Exception { doTest("tests/conformance/context/premultiplyalpha-test.html"); }
    public void test_conformance_context_resource_sharing_test_html() throws Exception { doTest("tests/conformance/context/resource-sharing-test.html"); }
    public void test_conformance_extensions_oes_standard_derivatives_html() throws Exception { doTest("tests/conformance/extensions/oes-standard-derivatives.html"); }
    public void test_conformance_extensions_oes_texture_float_with_canvas_html() throws Exception { doTest("tests/conformance/extensions/oes-texture-float-with-canvas.html"); }
    public void test_conformance_extensions_oes_texture_float_with_image_data_html() throws Exception { doTest("tests/conformance/extensions/oes-texture-float-with-image-data.html"); }
    public void test_conformance_extensions_oes_texture_float_with_image_html() throws Exception { doTest("tests/conformance/extensions/oes-texture-float-with-image.html"); }
    public void test_conformance_extensions_oes_texture_float_with_video_html() throws Exception { doTest("tests/conformance/extensions/oes-texture-float-with-video.html"); }
    public void test_conformance_extensions_oes_texture_float_html() throws Exception { doTest("tests/conformance/extensions/oes-texture-float.html"); }
    public void test_conformance_extensions_oes_vertex_array_object_html() throws Exception { doTest("tests/conformance/extensions/oes-vertex-array-object.html"); }
    public void test_conformance_extensions_webgl_debug_renderer_info_html() throws Exception { doTest("tests/conformance/extensions/webgl-debug-renderer-info.html"); }
    public void test_conformance_extensions_webgl_debug_shaders_html() throws Exception { doTest("tests/conformance/extensions/webgl-debug-shaders.html"); }
    public void test_conformance_glsl_functions_glsl_function_abs_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-abs.html"); }
    public void test_conformance_glsl_functions_glsl_function_acos_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-acos.html"); }
    public void test_conformance_glsl_functions_glsl_function_asin_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-asin.html"); }
    public void test_conformance_glsl_functions_glsl_function_atan_xy_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-atan-xy.html"); }
    public void test_conformance_glsl_functions_glsl_function_atan_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-atan.html"); }
    public void test_conformance_glsl_functions_glsl_function_ceil_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-ceil.html"); }
    public void test_conformance_glsl_functions_glsl_function_clamp_float_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-clamp-float.html"); }
    public void test_conformance_glsl_functions_glsl_function_clamp_gentype_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-clamp-gentype.html"); }
    public void test_conformance_glsl_functions_glsl_function_cos_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-cos.html"); }
    public void test_conformance_glsl_functions_glsl_function_cross_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-cross.html"); }
    public void test_conformance_glsl_functions_glsl_function_distance_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-distance.html"); }
    public void test_conformance_glsl_functions_glsl_function_dot_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-dot.html"); }
    public void test_conformance_glsl_functions_glsl_function_faceforward_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-faceforward.html"); }
    public void test_conformance_glsl_functions_glsl_function_floor_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-floor.html"); }
    public void test_conformance_glsl_functions_glsl_function_fract_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-fract.html"); }
    public void test_conformance_glsl_functions_glsl_function_length_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-length.html"); }
    public void test_conformance_glsl_functions_glsl_function_max_float_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-max-float.html"); }
    public void test_conformance_glsl_functions_glsl_function_max_gentype_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-max-gentype.html"); }
    public void test_conformance_glsl_functions_glsl_function_min_float_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-min-float.html"); }
    public void test_conformance_glsl_functions_glsl_function_min_gentype_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-min-gentype.html"); }
    public void test_conformance_glsl_functions_glsl_function_mix_float_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-mix-float.html"); }
    public void test_conformance_glsl_functions_glsl_function_mix_gentype_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-mix-gentype.html"); }
    public void test_conformance_glsl_functions_glsl_function_mod_float_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-mod-float.html"); }
    public void test_conformance_glsl_functions_glsl_function_mod_gentype_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-mod-gentype.html"); }
    public void test_conformance_glsl_functions_glsl_function_normalize_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-normalize.html"); }
    public void test_conformance_glsl_functions_glsl_function_reflect_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-reflect.html"); }
    public void test_conformance_glsl_functions_glsl_function_sign_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-sign.html"); }
    public void test_conformance_glsl_functions_glsl_function_sin_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-sin.html"); }
    public void test_conformance_glsl_functions_glsl_function_smoothstep_float_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-smoothstep-float.html"); }
    public void test_conformance_glsl_functions_glsl_function_smoothstep_gentype_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-smoothstep-gentype.html"); }
    public void test_conformance_glsl_functions_glsl_function_step_float_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-step-float.html"); }
    public void test_conformance_glsl_functions_glsl_function_step_gentype_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function-step-gentype.html"); }
    public void test_conformance_glsl_functions_glsl_function_html() throws Exception { doTest("tests/conformance/glsl/functions/glsl-function.html"); }
    public void test_conformance_glsl_implicit_add_int_float_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/add_int_float.vert.html"); }
    public void test_conformance_glsl_implicit_add_int_mat2_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/add_int_mat2.vert.html"); }
    public void test_conformance_glsl_implicit_add_int_mat3_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/add_int_mat3.vert.html"); }
    public void test_conformance_glsl_implicit_add_int_mat4_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/add_int_mat4.vert.html"); }
    public void test_conformance_glsl_implicit_add_int_vec2_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/add_int_vec2.vert.html"); }
    public void test_conformance_glsl_implicit_add_int_vec3_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/add_int_vec3.vert.html"); }
    public void test_conformance_glsl_implicit_add_int_vec4_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/add_int_vec4.vert.html"); }
    public void test_conformance_glsl_implicit_add_ivec2_vec2_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/add_ivec2_vec2.vert.html"); }
    public void test_conformance_glsl_implicit_add_ivec3_vec3_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/add_ivec3_vec3.vert.html"); }
    public void test_conformance_glsl_implicit_add_ivec4_vec4_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/add_ivec4_vec4.vert.html"); }
    public void test_conformance_glsl_implicit_assign_int_to_float_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/assign_int_to_float.vert.html"); }
    public void test_conformance_glsl_implicit_assign_ivec2_to_vec2_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/assign_ivec2_to_vec2.vert.html"); }
    public void test_conformance_glsl_implicit_assign_ivec3_to_vec3_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/assign_ivec3_to_vec3.vert.html"); }
    public void test_conformance_glsl_implicit_assign_ivec4_to_vec4_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/assign_ivec4_to_vec4.vert.html"); }
    public void test_conformance_glsl_implicit_construct_struct_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/construct_struct.vert.html"); }
    public void test_conformance_glsl_implicit_divide_int_float_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/divide_int_float.vert.html"); }
    public void test_conformance_glsl_implicit_divide_int_mat2_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/divide_int_mat2.vert.html"); }
    public void test_conformance_glsl_implicit_divide_int_mat3_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/divide_int_mat3.vert.html"); }
    public void test_conformance_glsl_implicit_divide_int_mat4_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/divide_int_mat4.vert.html"); }
    public void test_conformance_glsl_implicit_divide_int_vec2_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/divide_int_vec2.vert.html"); }
    public void test_conformance_glsl_implicit_divide_int_vec3_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/divide_int_vec3.vert.html"); }
    public void test_conformance_glsl_implicit_divide_int_vec4_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/divide_int_vec4.vert.html"); }
    public void test_conformance_glsl_implicit_divide_ivec2_vec2_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/divide_ivec2_vec2.vert.html"); }
    public void test_conformance_glsl_implicit_divide_ivec3_vec3_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/divide_ivec3_vec3.vert.html"); }
    public void test_conformance_glsl_implicit_divide_ivec4_vec4_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/divide_ivec4_vec4.vert.html"); }
    public void test_conformance_glsl_implicit_equal_int_float_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/equal_int_float.vert.html"); }
    public void test_conformance_glsl_implicit_equal_ivec2_vec2_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/equal_ivec2_vec2.vert.html"); }
    public void test_conformance_glsl_implicit_equal_ivec3_vec3_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/equal_ivec3_vec3.vert.html"); }
    public void test_conformance_glsl_implicit_equal_ivec4_vec4_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/equal_ivec4_vec4.vert.html"); }
    public void test_conformance_glsl_implicit_function_int_float_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/function_int_float.vert.html"); }
    public void test_conformance_glsl_implicit_function_ivec2_vec2_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/function_ivec2_vec2.vert.html"); }
    public void test_conformance_glsl_implicit_function_ivec3_vec3_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/function_ivec3_vec3.vert.html"); }
    public void test_conformance_glsl_implicit_function_ivec4_vec4_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/function_ivec4_vec4.vert.html"); }
    public void test_conformance_glsl_implicit_greater_than_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/greater_than.vert.html"); }
    public void test_conformance_glsl_implicit_greater_than_equal_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/greater_than_equal.vert.html"); }
    public void test_conformance_glsl_implicit_less_than_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/less_than.vert.html"); }
    public void test_conformance_glsl_implicit_less_than_equal_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/less_than_equal.vert.html"); }
    public void test_conformance_glsl_implicit_multiply_int_float_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/multiply_int_float.vert.html"); }
    public void test_conformance_glsl_implicit_multiply_int_mat2_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/multiply_int_mat2.vert.html"); }
    public void test_conformance_glsl_implicit_multiply_int_mat3_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/multiply_int_mat3.vert.html"); }
    public void test_conformance_glsl_implicit_multiply_int_mat4_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/multiply_int_mat4.vert.html"); }
    public void test_conformance_glsl_implicit_multiply_int_vec2_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/multiply_int_vec2.vert.html"); }
    public void test_conformance_glsl_implicit_multiply_int_vec3_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/multiply_int_vec3.vert.html"); }
    public void test_conformance_glsl_implicit_multiply_int_vec4_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/multiply_int_vec4.vert.html"); }
    public void test_conformance_glsl_implicit_multiply_ivec2_vec2_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/multiply_ivec2_vec2.vert.html"); }
    public void test_conformance_glsl_implicit_multiply_ivec3_vec3_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/multiply_ivec3_vec3.vert.html"); }
    public void test_conformance_glsl_implicit_multiply_ivec4_vec4_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/multiply_ivec4_vec4.vert.html"); }
    public void test_conformance_glsl_implicit_not_equal_int_float_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/not_equal_int_float.vert.html"); }
    public void test_conformance_glsl_implicit_not_equal_ivec2_vec2_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/not_equal_ivec2_vec2.vert.html"); }
    public void test_conformance_glsl_implicit_not_equal_ivec3_vec3_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/not_equal_ivec3_vec3.vert.html"); }
    public void test_conformance_glsl_implicit_not_equal_ivec4_vec4_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/not_equal_ivec4_vec4.vert.html"); }
    public void test_conformance_glsl_implicit_subtract_int_float_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/subtract_int_float.vert.html"); }
    public void test_conformance_glsl_implicit_subtract_int_mat2_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/subtract_int_mat2.vert.html"); }
    public void test_conformance_glsl_implicit_subtract_int_mat3_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/subtract_int_mat3.vert.html"); }
    public void test_conformance_glsl_implicit_subtract_int_mat4_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/subtract_int_mat4.vert.html"); }
    public void test_conformance_glsl_implicit_subtract_int_vec2_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/subtract_int_vec2.vert.html"); }
    public void test_conformance_glsl_implicit_subtract_int_vec3_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/subtract_int_vec3.vert.html"); }
    public void test_conformance_glsl_implicit_subtract_int_vec4_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/subtract_int_vec4.vert.html"); }
    public void test_conformance_glsl_implicit_subtract_ivec2_vec2_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/subtract_ivec2_vec2.vert.html"); }
    public void test_conformance_glsl_implicit_subtract_ivec3_vec3_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/subtract_ivec3_vec3.vert.html"); }
    public void test_conformance_glsl_implicit_subtract_ivec4_vec4_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/subtract_ivec4_vec4.vert.html"); }
    public void test_conformance_glsl_implicit_ternary_int_float_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/ternary_int_float.vert.html"); }
    public void test_conformance_glsl_implicit_ternary_ivec2_vec2_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/ternary_ivec2_vec2.vert.html"); }
    public void test_conformance_glsl_implicit_ternary_ivec3_vec3_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/ternary_ivec3_vec3.vert.html"); }
    public void test_conformance_glsl_implicit_ternary_ivec4_vec4_vert_html() throws Exception { doTest("tests/conformance/glsl/implicit/ternary_ivec4_vec4.vert.html"); }
    public void test_conformance_glsl_misc_attrib_location_length_limits_html() throws Exception { doTest("tests/conformance/glsl/misc/attrib-location-length-limits.html"); }
    public void test_conformance_glsl_misc_embedded_struct_definitions_forbidden_html() throws Exception { doTest("tests/conformance/glsl/misc/embedded-struct-definitions-forbidden.html"); }
    public void test_conformance_glsl_misc_empty_main_vert_html() throws Exception { doTest("tests/conformance/glsl/misc/empty_main.vert.html"); }
    public void test_conformance_glsl_misc_gl_position_unset_vert_html() throws Exception { doTest("tests/conformance/glsl/misc/gl_position_unset.vert.html"); }
    public void test_conformance_glsl_misc_glsl_function_nodes_html() throws Exception { doTest("tests/conformance/glsl/misc/glsl-function-nodes.html"); }
    public void test_conformance_glsl_misc_glsl_long_variable_names_html() throws Exception { doTest("tests/conformance/glsl/misc/glsl-long-variable-names.html"); }
    public void test_conformance_glsl_misc_non_ascii_comments_vert_html() throws Exception { doTest("tests/conformance/glsl/misc/non-ascii-comments.vert.html"); }
    public void test_conformance_glsl_misc_non_ascii_vert_html() throws Exception { doTest("tests/conformance/glsl/misc/non-ascii.vert.html"); }
    public void test_conformance_glsl_misc_shader_with_256_character_identifier_frag_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-256-character-identifier.frag.html"); }
    public void test_conformance_glsl_misc_shader_with_257_character_identifier_frag_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-257-character-identifier.frag.html"); }
    public void test_conformance_glsl_misc_shader_with__webgl_identifier_vert_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-_webgl-identifier.vert.html"); }
    public void test_conformance_glsl_misc_shader_with_arbitrary_indexing_frag_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-arbitrary-indexing.frag.html"); }
    public void test_conformance_glsl_misc_shader_with_arbitrary_indexing_vert_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-arbitrary-indexing.vert.html"); }
    public void test_conformance_glsl_misc_shader_with_attrib_array_vert_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-attrib-array.vert.html"); }
    public void test_conformance_glsl_misc_shader_with_attrib_struct_vert_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-attrib-struct.vert.html"); }
    public void test_conformance_glsl_misc_shader_with_clipvertex_vert_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-clipvertex.vert.html"); }
    public void test_conformance_glsl_misc_shader_with_default_precision_frag_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-default-precision.frag.html"); }
    public void test_conformance_glsl_misc_shader_with_default_precision_vert_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-default-precision.vert.html"); }
    public void test_conformance_glsl_misc_shader_with_define_line_continuation_frag_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-define-line-continuation.frag.html"); }
    public void test_conformance_glsl_misc_shader_with_dfdx_no_ext_frag_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-dfdx-no-ext.frag.html"); }
    public void test_conformance_glsl_misc_shader_with_dfdx_frag_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-dfdx.frag.html"); }
    public void test_conformance_glsl_misc_shader_with_error_directive_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-error-directive.html"); }
    public void test_conformance_glsl_misc_shader_with_explicit_int_cast_vert_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-explicit-int-cast.vert.html"); }
    public void test_conformance_glsl_misc_shader_with_float_return_value_frag_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-float-return-value.frag.html"); }
    public void test_conformance_glsl_misc_shader_with_frag_depth_frag_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-frag-depth.frag.html"); }
    public void test_conformance_glsl_misc_shader_with_function_recursion_frag_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-function-recursion.frag.html"); }
    public void test_conformance_glsl_misc_shader_with_glcolor_vert_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-glcolor.vert.html"); }
    public void test_conformance_glsl_misc_shader_with_gles_1_frag_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-gles-1.frag.html"); }
    public void test_conformance_glsl_misc_shader_with_gles_symbol_frag_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-gles-symbol.frag.html"); }
    public void test_conformance_glsl_misc_shader_with_glprojectionmatrix_vert_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-glprojectionmatrix.vert.html"); }
    public void test_conformance_glsl_misc_shader_with_implicit_vec3_to_vec4_cast_vert_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-implicit-vec3-to-vec4-cast.vert.html"); }
    public void test_conformance_glsl_misc_shader_with_include_vert_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-include.vert.html"); }
    public void test_conformance_glsl_misc_shader_with_int_return_value_frag_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-int-return-value.frag.html"); }
    public void test_conformance_glsl_misc_shader_with_invalid_identifier_frag_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-invalid-identifier.frag.html"); }
    public void test_conformance_glsl_misc_shader_with_ivec2_return_value_frag_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-ivec2-return-value.frag.html"); }
    public void test_conformance_glsl_misc_shader_with_ivec3_return_value_frag_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-ivec3-return-value.frag.html"); }
    public void test_conformance_glsl_misc_shader_with_ivec4_return_value_frag_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-ivec4-return-value.frag.html"); }
    public void test_conformance_glsl_misc_shader_with_limited_indexing_frag_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-limited-indexing.frag.html"); }
    public void test_conformance_glsl_misc_shader_with_long_line_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-long-line.html"); }
    public void test_conformance_glsl_misc_shader_with_non_ascii_error_frag_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-non-ascii-error.frag.html"); }
    public void test_conformance_glsl_misc_shader_with_precision_frag_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-precision.frag.html"); }
    public void test_conformance_glsl_misc_shader_with_quoted_error_frag_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-quoted-error.frag.html"); }
    public void test_conformance_glsl_misc_shader_with_undefined_preprocessor_symbol_frag_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-undefined-preprocessor-symbol.frag.html"); }
    public void test_conformance_glsl_misc_shader_with_uniform_in_loop_condition_vert_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-uniform-in-loop-condition.vert.html"); }
    public void test_conformance_glsl_misc_shader_with_vec2_return_value_frag_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-vec2-return-value.frag.html"); }
    public void test_conformance_glsl_misc_shader_with_vec3_return_value_frag_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-vec3-return-value.frag.html"); }
    public void test_conformance_glsl_misc_shader_with_vec4_return_value_frag_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-vec4-return-value.frag.html"); }
    public void test_conformance_glsl_misc_shader_with_version_100_frag_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-version-100.frag.html"); }
    public void test_conformance_glsl_misc_shader_with_version_100_vert_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-version-100.vert.html"); }
    public void test_conformance_glsl_misc_shader_with_version_120_vert_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-version-120.vert.html"); }
    public void test_conformance_glsl_misc_shader_with_version_130_vert_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-version-130.vert.html"); }
    public void test_conformance_glsl_misc_shader_with_webgl_identifier_vert_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-with-webgl-identifier.vert.html"); }
    public void test_conformance_glsl_misc_shader_without_precision_frag_html() throws Exception { doTest("tests/conformance/glsl/misc/shader-without-precision.frag.html"); }
    public void test_conformance_glsl_misc_shared_html() throws Exception { doTest("tests/conformance/glsl/misc/shared.html"); }
    public void test_conformance_glsl_misc_struct_nesting_exceeds_maximum_html() throws Exception { doTest("tests/conformance/glsl/misc/struct-nesting-exceeds-maximum.html"); }
    public void test_conformance_glsl_misc_struct_nesting_under_maximum_html() throws Exception { doTest("tests/conformance/glsl/misc/struct-nesting-under-maximum.html"); }
    public void test_conformance_glsl_misc_uniform_location_length_limits_html() throws Exception { doTest("tests/conformance/glsl/misc/uniform-location-length-limits.html"); }
    public void test_conformance_glsl_reserved__webgl_field_vert_html() throws Exception { doTest("tests/conformance/glsl/reserved/_webgl_field.vert.html"); }
    public void test_conformance_glsl_reserved__webgl_function_vert_html() throws Exception { doTest("tests/conformance/glsl/reserved/_webgl_function.vert.html"); }
    public void test_conformance_glsl_reserved__webgl_struct_vert_html() throws Exception { doTest("tests/conformance/glsl/reserved/_webgl_struct.vert.html"); }
    public void test_conformance_glsl_reserved__webgl_variable_vert_html() throws Exception { doTest("tests/conformance/glsl/reserved/_webgl_variable.vert.html"); }
    public void test_conformance_glsl_reserved_webgl_field_vert_html() throws Exception { doTest("tests/conformance/glsl/reserved/webgl_field.vert.html"); }
    public void test_conformance_glsl_reserved_webgl_function_vert_html() throws Exception { doTest("tests/conformance/glsl/reserved/webgl_function.vert.html"); }
    public void test_conformance_glsl_reserved_webgl_struct_vert_html() throws Exception { doTest("tests/conformance/glsl/reserved/webgl_struct.vert.html"); }
    public void test_conformance_glsl_reserved_webgl_variable_vert_html() throws Exception { doTest("tests/conformance/glsl/reserved/webgl_variable.vert.html"); }
    public void test_conformance_glsl_variables_gl_fragcoord_html() throws Exception { doTest("tests/conformance/glsl/variables/gl-fragcoord.html"); }
    public void test_conformance_glsl_variables_gl_frontfacing_html() throws Exception { doTest("tests/conformance/glsl/variables/gl-frontfacing.html"); }
    public void test_conformance_glsl_variables_gl_pointcoord_html() throws Exception { doTest("tests/conformance/glsl/variables/gl-pointcoord.html"); }
    public void test_conformance_limits_gl_max_texture_dimensions_html() throws Exception { doTest("tests/conformance/limits/gl-max-texture-dimensions.html"); }
    public void test_conformance_limits_gl_min_attribs_html() throws Exception { doTest("tests/conformance/limits/gl-min-attribs.html"); }
    public void test_conformance_limits_gl_min_textures_html() throws Exception { doTest("tests/conformance/limits/gl-min-textures.html"); }
    public void test_conformance_limits_gl_min_uniforms_html() throws Exception { doTest("tests/conformance/limits/gl-min-uniforms.html"); }
    public void test_conformance_misc_bad_arguments_test_html() throws Exception { doTest("tests/conformance/misc/bad-arguments-test.html"); }
    public void test_conformance_misc_error_reporting_html() throws Exception { doTest("tests/conformance/misc/error-reporting.html"); }
    public void test_conformance_misc_functions_returning_strings_html() throws Exception { doTest("tests/conformance/misc/functions-returning-strings.html"); }
    public void test_conformance_misc_instanceof_test_html() throws Exception { doTest("tests/conformance/misc/instanceof-test.html"); }
    public void test_conformance_misc_invalid_passed_params_html() throws Exception { doTest("tests/conformance/misc/invalid-passed-params.html"); }
    public void test_conformance_misc_is_object_html() throws Exception { doTest("tests/conformance/misc/is-object.html"); }
    public void test_conformance_misc_null_object_behaviour_html() throws Exception { doTest("tests/conformance/misc/null-object-behaviour.html"); }
    public void test_conformance_misc_object_deletion_behaviour_html() throws Exception { doTest("tests/conformance/misc/object-deletion-behaviour.html"); }
    public void test_conformance_misc_shader_precision_format_html() throws Exception { doTest("tests/conformance/misc/shader-precision-format.html"); }
    public void test_conformance_misc_type_conversion_test_html() throws Exception { doTest("tests/conformance/misc/type-conversion-test.html"); }
    public void test_conformance_misc_uninitialized_test_html() throws Exception { doTest("tests/conformance/misc/uninitialized-test.html"); }
    public void test_conformance_misc_webgl_specific_html() throws Exception { doTest("tests/conformance/misc/webgl-specific.html"); }
    public void test_conformance_more_conformance_constants_html() throws Exception { doTest("tests/conformance/more/conformance/constants.html"); }
    public void test_conformance_more_conformance_getContext_html() throws Exception { doTest("tests/conformance/more/conformance/getContext.html"); }
    public void test_conformance_more_conformance_methods_html() throws Exception { doTest("tests/conformance/more/conformance/methods.html"); }
    public void test_conformance_more_conformance_quickCheckAPI_A_html() throws Exception { doTest("tests/conformance/more/conformance/quickCheckAPI-A.html"); }
    public void test_conformance_more_conformance_quickCheckAPI_B1_html() throws Exception { doTest("tests/conformance/more/conformance/quickCheckAPI-B1.html"); }
    public void test_conformance_more_conformance_quickCheckAPI_B2_html() throws Exception { doTest("tests/conformance/more/conformance/quickCheckAPI-B2.html"); }
    public void test_conformance_more_conformance_quickCheckAPI_B3_html() throws Exception { doTest("tests/conformance/more/conformance/quickCheckAPI-B3.html"); }
    public void test_conformance_more_conformance_quickCheckAPI_B4_html() throws Exception { doTest("tests/conformance/more/conformance/quickCheckAPI-B4.html"); }
    public void test_conformance_more_conformance_quickCheckAPI_C_html() throws Exception { doTest("tests/conformance/more/conformance/quickCheckAPI-C.html"); }
    public void test_conformance_more_conformance_quickCheckAPI_D_G_html() throws Exception { doTest("tests/conformance/more/conformance/quickCheckAPI-D_G.html"); }
    public void test_conformance_more_conformance_quickCheckAPI_G_I_html() throws Exception { doTest("tests/conformance/more/conformance/quickCheckAPI-G_I.html"); }
    public void test_conformance_more_conformance_quickCheckAPI_L_S_html() throws Exception { doTest("tests/conformance/more/conformance/quickCheckAPI-L_S.html"); }
    public void test_conformance_more_conformance_quickCheckAPI_S_V_html() throws Exception { doTest("tests/conformance/more/conformance/quickCheckAPI-S_V.html"); }
    public void test_conformance_more_conformance_webGLArrays_html() throws Exception { doTest("tests/conformance/more/conformance/webGLArrays.html"); }
    public void test_conformance_more_functions_bindBuffer_html() throws Exception { doTest("tests/conformance/more/functions/bindBuffer.html"); }
    public void test_conformance_more_functions_bindBufferBadArgs_html() throws Exception { doTest("tests/conformance/more/functions/bindBufferBadArgs.html"); }
    public void test_conformance_more_functions_bindFramebufferLeaveNonZero_html() throws Exception { doTest("tests/conformance/more/functions/bindFramebufferLeaveNonZero.html"); }
    public void test_conformance_more_functions_bufferData_html() throws Exception { doTest("tests/conformance/more/functions/bufferData.html"); }
    public void test_conformance_more_functions_bufferDataBadArgs_html() throws Exception { doTest("tests/conformance/more/functions/bufferDataBadArgs.html"); }
    public void test_conformance_more_functions_bufferSubData_html() throws Exception { doTest("tests/conformance/more/functions/bufferSubData.html"); }
    public void test_conformance_more_functions_bufferSubDataBadArgs_html() throws Exception { doTest("tests/conformance/more/functions/bufferSubDataBadArgs.html"); }
    public void test_conformance_more_functions_copyTexImage2D_html() throws Exception { doTest("tests/conformance/more/functions/copyTexImage2D.html"); }
    public void test_conformance_more_functions_copyTexImage2DBadArgs_html() throws Exception { doTest("tests/conformance/more/functions/copyTexImage2DBadArgs.html"); }
    public void test_conformance_more_functions_copyTexSubImage2D_html() throws Exception { doTest("tests/conformance/more/functions/copyTexSubImage2D.html"); }
    public void test_conformance_more_functions_copyTexSubImage2DBadArgs_html() throws Exception { doTest("tests/conformance/more/functions/copyTexSubImage2DBadArgs.html"); }
    public void test_conformance_more_functions_deleteBufferBadArgs_html() throws Exception { doTest("tests/conformance/more/functions/deleteBufferBadArgs.html"); }
    public void test_conformance_more_functions_drawArrays_html() throws Exception { doTest("tests/conformance/more/functions/drawArrays.html"); }
    public void test_conformance_more_functions_drawArraysOutOfBounds_html() throws Exception { doTest("tests/conformance/more/functions/drawArraysOutOfBounds.html"); }
    public void test_conformance_more_functions_drawElements_html() throws Exception { doTest("tests/conformance/more/functions/drawElements.html"); }
    public void test_conformance_more_functions_drawElementsBadArgs_html() throws Exception { doTest("tests/conformance/more/functions/drawElementsBadArgs.html"); }
    public void test_conformance_more_functions_isTests_html() throws Exception { doTest("tests/conformance/more/functions/isTests.html"); }
    public void test_conformance_more_functions_readPixels_html() throws Exception { doTest("tests/conformance/more/functions/readPixels.html"); }
    public void test_conformance_more_functions_readPixelsBadArgs_html() throws Exception { doTest("tests/conformance/more/functions/readPixelsBadArgs.html"); }
    public void test_conformance_more_functions_texImage2D_html() throws Exception { doTest("tests/conformance/more/functions/texImage2D.html"); }
    public void test_conformance_more_functions_texImage2DBadArgs_html() throws Exception { doTest("tests/conformance/more/functions/texImage2DBadArgs.html"); }
    public void test_conformance_more_functions_texImage2DHTML_html() throws Exception { doTest("tests/conformance/more/functions/texImage2DHTML.html"); }
    public void test_conformance_more_functions_texImage2DHTMLBadArgs_html() throws Exception { doTest("tests/conformance/more/functions/texImage2DHTMLBadArgs.html"); }
    public void test_conformance_more_functions_texSubImage2D_html() throws Exception { doTest("tests/conformance/more/functions/texSubImage2D.html"); }
    public void test_conformance_more_functions_texSubImage2DBadArgs_html() throws Exception { doTest("tests/conformance/more/functions/texSubImage2DBadArgs.html"); }
    public void test_conformance_more_functions_texSubImage2DHTML_html() throws Exception { doTest("tests/conformance/more/functions/texSubImage2DHTML.html"); }
    public void test_conformance_more_functions_texSubImage2DHTMLBadArgs_html() throws Exception { doTest("tests/conformance/more/functions/texSubImage2DHTMLBadArgs.html"); }
    public void test_conformance_more_functions_uniformMatrix_html() throws Exception { doTest("tests/conformance/more/functions/uniformMatrix.html"); }
    public void test_conformance_more_functions_uniformMatrixBadArgs_html() throws Exception { doTest("tests/conformance/more/functions/uniformMatrixBadArgs.html"); }
    public void test_conformance_more_functions_uniformf_html() throws Exception { doTest("tests/conformance/more/functions/uniformf.html"); }
    public void test_conformance_more_functions_uniformfArrayLen1_html() throws Exception { doTest("tests/conformance/more/functions/uniformfArrayLen1.html"); }
    public void test_conformance_more_functions_uniformfBadArgs_html() throws Exception { doTest("tests/conformance/more/functions/uniformfBadArgs.html"); }
    public void test_conformance_more_functions_uniformi_html() throws Exception { doTest("tests/conformance/more/functions/uniformi.html"); }
    public void test_conformance_more_functions_uniformiBadArgs_html() throws Exception { doTest("tests/conformance/more/functions/uniformiBadArgs.html"); }
    public void test_conformance_more_functions_vertexAttrib_html() throws Exception { doTest("tests/conformance/more/functions/vertexAttrib.html"); }
    public void test_conformance_more_functions_vertexAttribBadArgs_html() throws Exception { doTest("tests/conformance/more/functions/vertexAttribBadArgs.html"); }
    public void test_conformance_more_functions_vertexAttribPointer_html() throws Exception { doTest("tests/conformance/more/functions/vertexAttribPointer.html"); }
    public void test_conformance_more_functions_vertexAttribPointerBadArgs_html() throws Exception { doTest("tests/conformance/more/functions/vertexAttribPointerBadArgs.html"); }
    public void test_conformance_more_glsl_arrayOutOfBounds_html() throws Exception { doTest("tests/conformance/more/glsl/arrayOutOfBounds.html"); }
    public void test_conformance_more_glsl_uniformOutOfBounds_html() throws Exception { doTest("tests/conformance/more/glsl/uniformOutOfBounds.html"); }
    public void test_conformance_programs_get_active_test_html() throws Exception { doTest("tests/conformance/programs/get-active-test.html"); }
    public void test_conformance_programs_gl_bind_attrib_location_test_html() throws Exception { doTest("tests/conformance/programs/gl-bind-attrib-location-test.html"); }
    public void test_conformance_programs_gl_get_active_attribute_html() throws Exception { doTest("tests/conformance/programs/gl-get-active-attribute.html"); }
    public void test_conformance_programs_gl_get_active_uniform_html() throws Exception { doTest("tests/conformance/programs/gl-get-active-uniform.html"); }
    public void test_conformance_programs_gl_getshadersource_html() throws Exception { doTest("tests/conformance/programs/gl-getshadersource.html"); }
    public void test_conformance_programs_gl_shader_test_html() throws Exception { doTest("tests/conformance/programs/gl-shader-test.html"); }
    public void test_conformance_programs_invalid_UTF_16_html() throws Exception { doTest("tests/conformance/programs/invalid-UTF-16.html"); }
    public void test_conformance_programs_program_test_html() throws Exception { doTest("tests/conformance/programs/program-test.html"); }
    public void test_conformance_reading_read_pixels_pack_alignment_html() throws Exception { doTest("tests/conformance/reading/read-pixels-pack-alignment.html"); }
    public void test_conformance_reading_read_pixels_test_html() throws Exception { doTest("tests/conformance/reading/read-pixels-test.html"); }
    public void test_conformance_renderbuffers_framebuffer_object_attachment_html() throws Exception { doTest("tests/conformance/renderbuffers/framebuffer-object-attachment.html"); }
    public void test_conformance_renderbuffers_framebuffer_test_html() throws Exception { doTest("tests/conformance/renderbuffers/framebuffer-test.html"); }
    public void test_conformance_renderbuffers_renderbuffer_initialization_html() throws Exception { doTest("tests/conformance/renderbuffers/renderbuffer-initialization.html"); }
    public void test_conformance_rendering_draw_arrays_out_of_bounds_html() throws Exception { doTest("tests/conformance/rendering/draw-arrays-out-of-bounds.html"); }
    public void test_conformance_rendering_draw_elements_out_of_bounds_html() throws Exception { doTest("tests/conformance/rendering/draw-elements-out-of-bounds.html"); }
    public void test_conformance_rendering_gl_clear_html() throws Exception { doTest("tests/conformance/rendering/gl-clear.html"); }
    public void test_conformance_rendering_gl_drawelements_html() throws Exception { doTest("tests/conformance/rendering/gl-drawelements.html"); }
    public void test_conformance_rendering_gl_scissor_test_html() throws Exception { doTest("tests/conformance/rendering/gl-scissor-test.html"); }
    public void test_conformance_rendering_line_loop_tri_fan_html() throws Exception { doTest("tests/conformance/rendering/line-loop-tri-fan.html"); }
    public void test_conformance_rendering_more_than_65536_indices_html() throws Exception { doTest("tests/conformance/rendering/more-than-65536-indices.html"); }
    public void test_conformance_rendering_multisample_corruption_html() throws Exception { doTest("tests/conformance/rendering/multisample-corruption.html"); }
    public void test_conformance_rendering_point_size_html() throws Exception { doTest("tests/conformance/rendering/point-size.html"); }
    public void test_conformance_rendering_triangle_html() throws Exception { doTest("tests/conformance/rendering/triangle.html"); }
    public void test_conformance_state_gl_enable_enum_test_html() throws Exception { doTest("tests/conformance/state/gl-enable-enum-test.html"); }
    public void test_conformance_state_gl_enum_tests_html() throws Exception { doTest("tests/conformance/state/gl-enum-tests.html"); }
    public void test_conformance_state_gl_get_calls_html() throws Exception { doTest("tests/conformance/state/gl-get-calls.html"); }
    public void test_conformance_state_gl_geterror_html() throws Exception { doTest("tests/conformance/state/gl-geterror.html"); }
    public void test_conformance_state_gl_getstring_html() throws Exception { doTest("tests/conformance/state/gl-getstring.html"); }
    public void test_conformance_state_gl_object_get_calls_html() throws Exception { doTest("tests/conformance/state/gl-object-get-calls.html"); }
    public void test_conformance_textures_compressed_tex_image_html() throws Exception { doTest("tests/conformance/textures/compressed-tex-image.html"); }
    public void test_conformance_textures_copy_tex_image_and_sub_image_2d_html() throws Exception { doTest("tests/conformance/textures/copy-tex-image-and-sub-image-2d.html"); }
    public void test_conformance_textures_gl_pixelstorei_html() throws Exception { doTest("tests/conformance/textures/gl-pixelstorei.html"); }
    public void test_conformance_textures_gl_teximage_html() throws Exception { doTest("tests/conformance/textures/gl-teximage.html"); }
    public void test_conformance_textures_origin_clean_conformance_html() throws Exception { doTest("tests/conformance/textures/origin-clean-conformance.html"); }
    public void test_conformance_textures_tex_image_and_sub_image_2d_with_array_buffer_view_html() throws Exception { doTest("tests/conformance/textures/tex-image-and-sub-image-2d-with-array-buffer-view.html"); }
    public void test_conformance_textures_tex_image_and_sub_image_2d_with_canvas_rgb565_html() throws Exception { doTest("tests/conformance/textures/tex-image-and-sub-image-2d-with-canvas-rgb565.html"); }
    public void test_conformance_textures_tex_image_and_sub_image_2d_with_canvas_rgba4444_html() throws Exception { doTest("tests/conformance/textures/tex-image-and-sub-image-2d-with-canvas-rgba4444.html"); }
    public void test_conformance_textures_tex_image_and_sub_image_2d_with_canvas_rgba5551_html() throws Exception { doTest("tests/conformance/textures/tex-image-and-sub-image-2d-with-canvas-rgba5551.html"); }
    public void test_conformance_textures_tex_image_and_sub_image_2d_with_canvas_html() throws Exception { doTest("tests/conformance/textures/tex-image-and-sub-image-2d-with-canvas.html"); }
    public void test_conformance_textures_tex_image_and_sub_image_2d_with_image_data_rgb565_html() throws Exception { doTest("tests/conformance/textures/tex-image-and-sub-image-2d-with-image-data-rgb565.html"); }
    public void test_conformance_textures_tex_image_and_sub_image_2d_with_image_data_rgba4444_html() throws Exception { doTest("tests/conformance/textures/tex-image-and-sub-image-2d-with-image-data-rgba4444.html"); }
    public void test_conformance_textures_tex_image_and_sub_image_2d_with_image_data_rgba5551_html() throws Exception { doTest("tests/conformance/textures/tex-image-and-sub-image-2d-with-image-data-rgba5551.html"); }
    public void test_conformance_textures_tex_image_and_sub_image_2d_with_image_data_html() throws Exception { doTest("tests/conformance/textures/tex-image-and-sub-image-2d-with-image-data.html"); }
    public void test_conformance_textures_tex_image_and_sub_image_2d_with_image_rgb565_html() throws Exception { doTest("tests/conformance/textures/tex-image-and-sub-image-2d-with-image-rgb565.html"); }
    public void test_conformance_textures_tex_image_and_sub_image_2d_with_image_rgba4444_html() throws Exception { doTest("tests/conformance/textures/tex-image-and-sub-image-2d-with-image-rgba4444.html"); }
    public void test_conformance_textures_tex_image_and_sub_image_2d_with_image_rgba5551_html() throws Exception { doTest("tests/conformance/textures/tex-image-and-sub-image-2d-with-image-rgba5551.html"); }
    public void test_conformance_textures_tex_image_and_sub_image_2d_with_image_html() throws Exception { doTest("tests/conformance/textures/tex-image-and-sub-image-2d-with-image.html"); }
    public void test_conformance_textures_tex_image_and_sub_image_2d_with_video_rgb565_html() throws Exception { doTest("tests/conformance/textures/tex-image-and-sub-image-2d-with-video-rgb565.html"); }
    public void test_conformance_textures_tex_image_and_sub_image_2d_with_video_rgba4444_html() throws Exception { doTest("tests/conformance/textures/tex-image-and-sub-image-2d-with-video-rgba4444.html"); }
    public void test_conformance_textures_tex_image_and_sub_image_2d_with_video_rgba5551_html() throws Exception { doTest("tests/conformance/textures/tex-image-and-sub-image-2d-with-video-rgba5551.html"); }
    public void test_conformance_textures_tex_image_and_sub_image_2d_with_video_html() throws Exception { doTest("tests/conformance/textures/tex-image-and-sub-image-2d-with-video.html"); }
    public void test_conformance_textures_tex_image_and_uniform_binding_bugs_html() throws Exception { doTest("tests/conformance/textures/tex-image-and-uniform-binding-bugs.html"); }
    public void test_conformance_textures_tex_image_with_format_and_type_html() throws Exception { doTest("tests/conformance/textures/tex-image-with-format-and-type.html"); }
    public void test_conformance_textures_tex_image_with_invalid_data_html() throws Exception { doTest("tests/conformance/textures/tex-image-with-invalid-data.html"); }
    public void test_conformance_textures_tex_input_validation_html() throws Exception { doTest("tests/conformance/textures/tex-input-validation.html"); }
    public void test_conformance_textures_tex_sub_image_2d_bad_args_html() throws Exception { doTest("tests/conformance/textures/tex-sub-image-2d-bad-args.html"); }
    public void test_conformance_textures_tex_sub_image_2d_html() throws Exception { doTest("tests/conformance/textures/tex-sub-image-2d.html"); }
    public void test_conformance_textures_texparameter_test_html() throws Exception { doTest("tests/conformance/textures/texparameter-test.html"); }
    public void test_conformance_textures_texture_active_bind_2_html() throws Exception { doTest("tests/conformance/textures/texture-active-bind-2.html"); }
    public void test_conformance_textures_texture_active_bind_html() throws Exception { doTest("tests/conformance/textures/texture-active-bind.html"); }
    public void test_conformance_textures_texture_complete_html() throws Exception { doTest("tests/conformance/textures/texture-complete.html"); }
    public void test_conformance_textures_texture_mips_html() throws Exception { doTest("tests/conformance/textures/texture-mips.html"); }
    public void test_conformance_textures_texture_npot_video_html() throws Exception { doTest("tests/conformance/textures/texture-npot-video.html"); }
    public void test_conformance_textures_texture_npot_html() throws Exception { doTest("tests/conformance/textures/texture-npot.html"); }
    public void test_conformance_textures_texture_size_cube_maps_html() throws Exception { doTest("tests/conformance/textures/texture-size-cube-maps.html"); }
    public void test_conformance_textures_texture_size_html() throws Exception { doTest("tests/conformance/textures/texture-size.html"); }
    public void test_conformance_textures_texture_transparent_pixels_initialized_html() throws Exception { doTest("tests/conformance/textures/texture-transparent-pixels-initialized.html"); }
    public void test_conformance_typedarrays_array_buffer_crash_html() throws Exception { doTest("tests/conformance/typedarrays/array-buffer-crash.html"); }
    public void test_conformance_typedarrays_array_buffer_view_crash_html() throws Exception { doTest("tests/conformance/typedarrays/array-buffer-view-crash.html"); }
    public void test_conformance_typedarrays_array_unit_tests_html() throws Exception { doTest("tests/conformance/typedarrays/array-unit-tests.html"); }
    public void test_conformance_typedarrays_data_view_crash_html() throws Exception { doTest("tests/conformance/typedarrays/data-view-crash.html"); }
    public void test_conformance_typedarrays_data_view_test_html() throws Exception { doTest("tests/conformance/typedarrays/data-view-test.html"); }
    public void test_conformance_uniforms_gl_uniform_arrays_html() throws Exception { doTest("tests/conformance/uniforms/gl-uniform-arrays.html"); }
    public void test_conformance_uniforms_gl_uniform_bool_html() throws Exception { doTest("tests/conformance/uniforms/gl-uniform-bool.html"); }
    public void test_conformance_uniforms_gl_uniformmatrix4fv_html() throws Exception { doTest("tests/conformance/uniforms/gl-uniformmatrix4fv.html"); }
    public void test_conformance_uniforms_gl_unknown_uniform_html() throws Exception { doTest("tests/conformance/uniforms/gl-unknown-uniform.html"); }
    public void test_conformance_uniforms_null_uniform_location_html() throws Exception { doTest("tests/conformance/uniforms/null-uniform-location.html"); }
    public void test_conformance_uniforms_uniform_location_html() throws Exception { doTest("tests/conformance/uniforms/uniform-location.html"); }
    public void test_conformance_uniforms_uniform_samplers_test_html() throws Exception { doTest("tests/conformance/uniforms/uniform-samplers-test.html"); }
}
