Spectrum eval_op::operator()(const DisneyDiffuse &bsdf) const {
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
    Real eval_subsurface = eval(bsdf.subsurface, vertex.uv, vertex.uv_screen_size, texture_pool);
    Vector3 h      = normalize(dir_in + dir_out);
    Real half_angle = abs(dot(h, dir_out));
    Real in_angle   = abs(dot(frame.n, dir_in));
    Real out_angle  = abs(dot(frame.n, dir_out)); 

    // Modified Schlick Fresnel
    Real F_D90 = Real(1) / Real(2) + Real(2) * eval_roughness * pow(half_angle, Real(2));
    Real F_D_w_in = Real(1) + (F_D90 - Real(1)) * (Real(1) - pow(in_angle, 5));
    Real F_D_w_out = Real(1) + (F_D90 - Real(1)) * (Real(1) - pow(out_angle, 5));

    // Base Diffuse
    Spectrum f_baseDiffuse = (eval_base_color / c_PI) * F_D_w_in * F_D_w_out * out_angle;

    // Lommel-Seeliger
    Real F_SS90 = eval_roughness * pow(half_angle, 2);
    Real F_SS_w_in = Real(1) + (F_SS90 - 1) * pow((1 - in_angle), 5);
    Real F_SS_w_out = Real(1) + (F_SS90 - 1) * pow((1 - out_angle), 5);
    Real absorbtion = 1 / (in_angle + out_angle);

    // Subsurface
    Spectrum f_subsurface = (Real(1.25) * eval_base_color / c_PI) *
        (F_SS_w_in * F_SS_w_out * (absorbtion - Real(0.5)) + Real(0.5)) * out_angle;

    // Final
    Spectrum f_diffuse = (Real(1) - eval_subsurface) * f_baseDiffuse + eval_subsurface * f_subsurface;

    return f_diffuse;
}

Real pdf_sample_bsdf_op::operator()(const DisneyDiffuse &bsdf) const {
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
    // importance sample the cosine hemisphere domain.
    return fmax(dot(frame.n, dir_out), Real(0)) / c_PI;
}

std::optional<BSDFSampleRecord> sample_bsdf_op::operator()(const DisneyDiffuse &bsdf) const {
    if (dot(vertex.geometric_normal, dir_in) < 0) {
        // No light below the surface
        return {};
    }
    // Flip the shading frame if it is inconsistent with the geometry normal
    Frame frame = vertex.shading_frame;
    if (dot(frame.n, dir_in) < 0) {
        frame = -frame;
    }

    Real eval_roughness = eval(bsdf.roughness, vertex.uv, vertex.uv_screen_size, texture_pool);
    eval_roughness = std::clamp(eval_roughness, Real(0.01), Real(1));
    // Homework 1: implement this!
    return BSDFSampleRecord{
        to_world(frame, sample_cos_hemisphere(rnd_param_uv)),
        Real(0) /* eta */, eval_roughness /* roughness */ };
}

TextureSpectrum get_texture_op::operator()(const DisneyDiffuse &bsdf) const {
    return bsdf.base_color;
}
