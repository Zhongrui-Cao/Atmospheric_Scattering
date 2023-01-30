#include "../microfacet.h"

Spectrum eval_op::operator()(const DisneyMetal &bsdf) const {
    if (dot(vertex.geometric_normal, dir_in) < 0 ||
            dot(vertex.geometric_normal, dir_out) < 0) {
        // No light below the surface
        return make_zero_spectrum();
    }
    // Flip the shading frame if it is inconsistent with the geometry normal
    Frame frame = vertex.shading_frame;
    if (dot(frame.n, dir_in) < 0) {
        frame = -frame;
    }
    // Homework 1: implement this!
    
    // vars
    Spectrum eval_base_color = eval(bsdf.base_color, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real eval_roughness = eval(bsdf.roughness, vertex.uv, vertex.uv_screen_size, texture_pool);
    eval_roughness = std::clamp(eval_roughness, Real(0.01), Real(1));
    Real eval_anisotropic = eval(bsdf.anisotropic, vertex.uv, vertex.uv_screen_size, texture_pool);
    Vector3 h = normalize(dir_in + dir_out);
    Real half_angle = abs(dot(h, dir_out));
    Real in_angle = abs(dot(frame.n, dir_in));

    // Schlick
    Spectrum F_m = schlick_fresnel(eval_base_color, half_angle);

    // Normal distribution function GGX
    Vector3 h_l = to_local(frame, h);
    Real D_m = Disney_GGX(eval_anisotropic, eval_roughness, h_l);

    // Smith Masking-shadowing
    Real G_w_in  = Disney_Smith(eval_anisotropic, eval_roughness, dir_in, frame);
    Real G_w_out = Disney_Smith(eval_anisotropic, eval_roughness, dir_out, frame);
    Real G_m     = G_w_in * G_w_out;

    // Cook-Torrance BRDF
    Spectrum f_metal = (F_m * D_m * G_m) / (4 * in_angle);

    return f_metal;
}

Real pdf_sample_bsdf_op::operator()(const DisneyMetal &bsdf) const {
    if (dot(vertex.geometric_normal, dir_in) < 0 ||
            dot(vertex.geometric_normal, dir_out) < 0) {
        // No light below the surface
        return 0;
    }
    // Flip the shading frame if it is inconsistent with the geometry normal
    Frame frame = vertex.shading_frame;
    if (dot(frame.n, dir_in) < 0) {
        frame = -frame;
    }
    // Homework 1: implement this!
    // vars
    Real eval_roughness = eval(bsdf.roughness, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real eval_anisotropic = eval(bsdf.anisotropic, vertex.uv, vertex.uv_screen_size, texture_pool);
    Vector3 h = normalize(dir_in + dir_out);
    Real in_angle = abs(dot(frame.n, dir_in));

    // Normal distribution function GGX
    Vector3 h_l = to_local(frame, h);
    Real D_m = Disney_GGX(eval_anisotropic, eval_roughness, h_l);

    // Smith Masking-shadowing
    Real G_w_in = Disney_Smith(eval_anisotropic, eval_roughness, dir_in, frame);

    Real pdf = (D_m * G_w_in) / (4 * in_angle);

    return pdf;
}

std::optional<BSDFSampleRecord>
        sample_bsdf_op::operator()(const DisneyMetal &bsdf) const {
    if (dot(vertex.geometric_normal, dir_in) < 0) {
        // No light below the surface
        return {};
    }
    // Flip the shading frame if it is inconsistent with the geometry normal
    Frame frame = vertex.shading_frame;
    if (dot(frame.n, dir_in) < 0) {
        frame = -frame;
    }
    // Homework 1: implement this!

    // Convert the incoming direction to local coordinates
    Vector3 local_dir_in = to_local(frame, dir_in);
    Real roughness = eval(
        bsdf.roughness, vertex.uv, vertex.uv_screen_size, texture_pool);
    // Clamp roughness to avoid numerical issues.
    roughness = std::clamp(roughness, Real(0.01), Real(1));

    Real eval_anisotropic = eval(bsdf.anisotropic, vertex.uv, vertex.uv_screen_size, texture_pool);

    Real aspect = sqrt(Real(1) - Real(0.9) * eval_anisotropic);
    Real a_min = Real(0.0001);
    Real a_x = max(a_min, pow(roughness, 2) / aspect);
    Real a_y = max(a_min, pow(roughness, 2) * aspect);

    Vector3 local_micro_normal = sampleGGXVNDF(local_dir_in, a_x, a_y, rnd_param_uv);

    // Transform the micro normal to world space
    Vector3 half_vector = to_world(frame, local_micro_normal);
    // Reflect over the world space normal
    Vector3 reflected = normalize(-dir_in + 2 * dot(dir_in, half_vector) * half_vector);
    return BSDFSampleRecord{
        reflected,
        Real(0) /* eta */, roughness /* roughness */
    };
}

TextureSpectrum get_texture_op::operator()(const DisneyMetal &bsdf) const {
    return bsdf.base_color;
}
