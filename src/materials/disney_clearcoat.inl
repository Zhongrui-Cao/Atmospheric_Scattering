#include "../microfacet.h"

Spectrum eval_op::operator()(const DisneyClearcoat &bsdf) const {
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
    Real clearcoatGloss = eval(bsdf.clearcoat_gloss, vertex.uv, vertex.uv_screen_size, texture_pool);
    Vector3 h = normalize(dir_in + dir_out);
    Real half_angle = abs(dot(h, dir_out));
    Real in_angle = abs(dot(frame.n, dir_in));

    // Schlick Fresnel
    Real R_0 = pow(Real(1.5) - Real(1), 2) / pow(Real(1.5) + Real(1), 2);
    Real F_c = R_0 + (1 - R_0) * pow((1 - half_angle), 5);

    // Normal distribution function
    Vector3 h_l = to_local(frame, h);
    Real D_c = Clearcoat_GGX(clearcoatGloss, h_l);

    // Shadowing masking term
    Real G_c = Clearcoat_Smith(dir_in, frame) * Clearcoat_Smith(dir_out, frame);

    // Clearcoat BRDF
    Real f_clearcoat = (F_c * D_c * G_c) / (Real(4) * in_angle);

    return make_const_spectrum(f_clearcoat);
}

Real pdf_sample_bsdf_op::operator()(const DisneyClearcoat &bsdf) const {
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
    Real clearcoatGloss = eval(bsdf.clearcoat_gloss, vertex.uv, vertex.uv_screen_size, texture_pool);
    Vector3 h = normalize(dir_in + dir_out);
    Real half_angle = abs(dot(h, dir_out));

    // Normal distribution function
    Vector3 h_l = to_local(frame, h);
    Real D_c = Clearcoat_GGX(clearcoatGloss, h_l);

    return (D_c * abs(dot(frame.n, h))) / (Real(4) * half_angle);
}

std::optional<BSDFSampleRecord>
        sample_bsdf_op::operator()(const DisneyClearcoat &bsdf) const {
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
    Real clearcoatGloss = eval(bsdf.clearcoat_gloss, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real a_g = (Real(1) - clearcoatGloss) * Real(0.1) + clearcoatGloss * Real(0.001);
    Real a_g_2 = pow(a_g, 2);

    Real cos_h_elevation = sqrt((1- pow(a_g_2, (1 - rnd_param_uv.x))) / (1 - a_g_2));
    Real h_elevation = acos(cos_h_elevation);
    Real h_azimuth   = Real(2) * c_PI * rnd_param_uv.y;

    Vector3 h_l = Vector3();
    h_l.x = sin(h_elevation) * cos(h_azimuth);
    h_l.y = sin(h_elevation) * sin(h_azimuth);
    h_l.z = cos_h_elevation;

    // Transform the micro normal to world space
    Vector3 half_vector = to_world(frame, h_l);
    // Reflect over the world space normal
    Vector3 reflected = normalize(-dir_in + 2 * dot(dir_in, half_vector) * half_vector);

    return BSDFSampleRecord{
        reflected,
        Real(0) /* eta */, a_g /* roughness */ };
}

TextureSpectrum get_texture_op::operator()(const DisneyClearcoat &bsdf) const {
    return make_constant_spectrum_texture(make_zero_spectrum());
}
