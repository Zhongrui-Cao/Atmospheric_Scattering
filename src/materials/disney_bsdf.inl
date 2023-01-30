#include "../microfacet.h"

Spectrum eval_op::operator()(const DisneyBSDF &bsdf) const {
    bool reflect = dot(vertex.geometric_normal, dir_in) *
                   dot(vertex.geometric_normal, dir_out) > 0;
    // Flip the shading frame if it is inconsistent with the geometry normal
    Frame frame = vertex.shading_frame;
    if (dot(frame.n, dir_in) * dot(vertex.geometric_normal, dir_in) < 0) {
        frame = -frame;
    }
    // Homework 1: implement this!

    eval_op evaluate = { dir_in, dir_out, vertex, texture_pool, dir };
    
    DisneyDiffuse   diffuse   = { bsdf.base_color, bsdf.roughness, bsdf.subsurface };
    DisneySheen     sheen     = { bsdf.base_color, bsdf.sheen_tint };
    DisneyMetal     metal     = { bsdf.base_color, bsdf.roughness, bsdf.anisotropic };
    DisneyClearcoat clearcoat = { bsdf.clearcoat_gloss };
    DisneyGlass     glass     = { bsdf.base_color, bsdf.roughness, bsdf.anisotropic, bsdf.eta };

    // vars
    Spectrum base_color = eval(bsdf.base_color, vertex.uv, vertex.uv_screen_size, texture_pool);
    Real roughness = eval(bsdf.roughness, vertex.uv, vertex.uv_screen_size, texture_pool);
    roughness = std::clamp(roughness, Real(0.01), Real(1));
    Real anisotropic = eval(bsdf.anisotropic, vertex.uv, vertex.uv_screen_size, texture_pool);
    anisotropic = std::clamp(anisotropic, Real(0.01), Real(1));
    Real specular_tint = eval(bsdf.specular_tint, vertex.uv, vertex.uv_screen_size, texture_pool);
    specular_tint = std::clamp(specular_tint, Real(0.01), Real(1));
    Real specular = eval(bsdf.specular, vertex.uv, vertex.uv_screen_size, texture_pool);
    specular = std::clamp(specular, Real(0.01), Real(1));
    Real metallic = eval(bsdf.metallic, vertex.uv, vertex.uv_screen_size, texture_pool);
    metallic = std::clamp(metallic, Real(0.01), Real(1));
    Real specular_transmission = eval(bsdf.specular_transmission, vertex.uv, vertex.uv_screen_size, texture_pool);
    specular_transmission = std::clamp(specular_transmission, Real(0.01), Real(1));
    Real sheen_var = eval(bsdf.sheen, vertex.uv, vertex.uv_screen_size, texture_pool);
    sheen_var = std::clamp(sheen_var, Real(0.01), Real(1));
    Real clearcoat_var = eval(bsdf.clearcoat, vertex.uv, vertex.uv_screen_size, texture_pool);
    clearcoat_var = std::clamp(clearcoat_var, Real(0.01), Real(1));
    
    Real eta = dot(vertex.geometric_normal, dir_in) > 0 ? bsdf.eta : 1 / bsdf.eta;
    Vector3 h = normalize(dir_in + dir_out);
    Real half_angle = abs(dot(h, dir_out));
    Real in_angle = abs(dot(frame.n, dir_in));

    //// For modified metal ////
    
    // Schlick with specular tint
    Spectrum F_hat_m = Fresnel_with_tint(base_color, specular_tint, specular,
        eta, metallic, dot(h, dir_out));

    // Normal distribution function GGX
    Vector3 h_l = to_local(frame, h);
    Real D_m = Disney_GGX(anisotropic, roughness, h_l);

    // Smith Masking-shadowing
    Real G_w_in = Disney_Smith(anisotropic, roughness, dir_in, frame);
    Real G_w_out = Disney_Smith(anisotropic, roughness, dir_out, frame);
    Real G_m = G_w_in * G_w_out;
    //// End modified metal ////

    Spectrum f_hat_metal;
    Spectrum f_diffuse;
    Spectrum f_sheen;
    Spectrum f_clearcoat;
    Spectrum f_glass = evaluate(glass);

    // When ray inside the object remove everything except glass
    if (dot(dir_in, vertex.geometric_normal) <= 0) {
        f_hat_metal = make_zero_spectrum();
        f_diffuse   = make_zero_spectrum();
        f_sheen     = make_zero_spectrum();
        f_clearcoat = make_zero_spectrum();
    }
    else {
        if (dot(vertex.geometric_normal, dir_out) < 0) {
            // No light below the surface
            // 3 hour bug
            f_hat_metal = make_zero_spectrum();
        }
        else {
            f_hat_metal = (F_hat_m * D_m * G_m) / (4 * in_angle);
        }
        f_diffuse   = evaluate(diffuse);
        f_sheen     = evaluate(sheen);
        f_clearcoat = evaluate(clearcoat);
    }

    Spectrum f_disney = (Real(1) - specular_transmission) * (Real(1) - metallic) * f_diffuse +  // diffuse
                        (Real(1) - metallic) * sheen_var * f_sheen +                            // sheen
                        (Real(1) - specular_transmission * (Real(1) - metallic)) * f_hat_metal +// metal
                        Real(0.25) * clearcoat_var * f_clearcoat +                              // clearcoat
                        (Real(1) - metallic) * specular_transmission * f_glass;                 // glass

    return f_disney;
}

Real pdf_sample_bsdf_op::operator()(const DisneyBSDF &bsdf) const {
    bool reflect = dot(vertex.geometric_normal, dir_in) *
                   dot(vertex.geometric_normal, dir_out) > 0;
    // Flip the shading frame if it is inconsistent with the geometry normal
    Frame frame = vertex.shading_frame;
    if (dot(frame.n, dir_in) * dot(vertex.geometric_normal, dir_in) < 0) {
        frame = -frame;
    }
    // Homework 1: implement this!
    (void)reflect; // silence unuse warning, remove this when implementing hw

    // vars
    Real metallic = eval(bsdf.metallic, vertex.uv, vertex.uv_screen_size, texture_pool);
    metallic = std::clamp(metallic, Real(0.01), Real(1));
    Real specular_transmission = eval(bsdf.specular_transmission, vertex.uv, vertex.uv_screen_size, texture_pool);
    specular_transmission = std::clamp(specular_transmission, Real(0.01), Real(1));
    Real clearcoat_var = eval(bsdf.clearcoat, vertex.uv, vertex.uv_screen_size, texture_pool);
    clearcoat_var = std::clamp(clearcoat_var, Real(0.01), Real(1));

    pdf_sample_bsdf_op pdf = { dir_in, dir_out, vertex, texture_pool, dir };

    DisneyDiffuse   diffuse   = { bsdf.base_color, bsdf.roughness, bsdf.subsurface };
    DisneySheen     sheen     = { bsdf.base_color, bsdf.sheen_tint };
    DisneyMetal     metal     = { bsdf.base_color, bsdf.roughness, bsdf.anisotropic };
    DisneyClearcoat clearcoat = { bsdf.clearcoat_gloss };
    DisneyGlass     glass     = { bsdf.base_color, bsdf.roughness, bsdf.anisotropic, bsdf.eta };

    Real diffuseWeight = (Real(1) - metallic) * (Real(1) - specular_transmission);
    Real metalWeight = (Real(1) - specular_transmission * (1 - metallic));
    Real glassWeight = (Real(1) - metallic) * specular_transmission;
    Real clearcoatWeight = Real(0.25) * clearcoat_var;

    Real total = diffuseWeight + metalWeight + glassWeight + clearcoatWeight;
    diffuseWeight = diffuseWeight / total;
    metalWeight = metalWeight / total;
    glassWeight = glassWeight / total;
    clearcoatWeight = clearcoatWeight / total;

    // if ray inside obj return glass lobe
    if (dot(dir_in, vertex.geometric_normal) <= 0) {
        return pdf(glass);
    }

    return pdf(diffuse) * diffuseWeight + pdf(metal) * metalWeight + pdf(glass) * glassWeight +
        pdf(clearcoat) * clearcoatWeight;
}

std::optional<BSDFSampleRecord>
sample_bsdf_op::operator()(const DisneyBSDF& bsdf) const {
    // Flip the shading frame if it is inconsistent with the geometry normal
    Frame frame = vertex.shading_frame;
    if (dot(frame.n, dir_in) * dot(vertex.geometric_normal, dir_in) < 0) {
        frame = -frame;
    }
    // Homework 1: implement this!

    // vars
    Real metallic = eval(bsdf.metallic, vertex.uv, vertex.uv_screen_size, texture_pool);
    metallic = std::clamp(metallic, Real(0.01), Real(1));
    Real specular_transmission = eval(bsdf.specular_transmission, vertex.uv, vertex.uv_screen_size, texture_pool);
    specular_transmission = std::clamp(specular_transmission, Real(0.01), Real(1));
    Real clearcoat_var = eval(bsdf.clearcoat, vertex.uv, vertex.uv_screen_size, texture_pool);
    clearcoat_var = std::clamp(clearcoat_var, Real(0.01), Real(1));

    sample_bsdf_op sample = { dir_in, vertex, texture_pool, rnd_param_uv, rnd_param_w, dir };

    DisneyDiffuse   diffuse   = { bsdf.base_color, bsdf.roughness, bsdf.subsurface };
    DisneySheen     sheen     = { bsdf.base_color, bsdf.sheen_tint };
    DisneyMetal     metal     = { bsdf.base_color, bsdf.roughness, bsdf.anisotropic };
    DisneyClearcoat clearcoat = { bsdf.clearcoat_gloss };
    DisneyGlass     glass     = { bsdf.base_color, bsdf.roughness, bsdf.anisotropic, bsdf.eta };

    Real diffuseWeight = (Real(1) - metallic) * (Real(1) - specular_transmission);
    Real metalWeight = (Real(1) - specular_transmission * (1 - metallic));
    Real glassWeight = (Real(1) - metallic) * specular_transmission;
    Real clearcoatWeight = Real(0.25) * clearcoat_var;

    Real total = diffuseWeight + metalWeight + glassWeight + clearcoatWeight;
    diffuseWeight = diffuseWeight / total;
    metalWeight = metalWeight / total;
    glassWeight = glassWeight / total;
    clearcoatWeight = clearcoatWeight / total;

    // if ray inside obj return glass lobe
    if (dot(dir_in, vertex.geometric_normal) <= 0) {
        return sample(glass);
    }

    //printf("d weight=%f, m weight=%f, g weight=%f, c weight=%f \n", diffuseWeight,
    //   metalWeight, glassWeight, clearcoatWeight);

    Real diffuse_cut = diffuseWeight;
    Real metal_cut = diffuse_cut + metalWeight;
    Real glass_cut = metal_cut + glassWeight;
    Real clear_cut = glass_cut + clearcoatWeight;

    //printf("random w=%f \n", rnd_param_w);

    if (rnd_param_w < diffuse_cut) {
        return sample(diffuse);
    }
    else if (diffuse_cut <= rnd_param_w && rnd_param_w < metal_cut) {
        return sample(metal);
    }
    else if (metal_cut <= rnd_param_w && rnd_param_w < glass_cut) {
        return sample(glass);
    }
    else{
        return sample(clearcoat);
    }

}

TextureSpectrum get_texture_op::operator()(const DisneyBSDF &bsdf) const {
    return bsdf.base_color;
}
