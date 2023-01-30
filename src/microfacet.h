#pragma once

#include "lajolla.h"
#include "spectrum.h"

/// A microfacet model assumes that the surface is composed of infinitely many little mirrors/glasses.
/// The orientation of the mirrors determines the amount of lights reflected.
/// The distribution of the orientation is determined empirically.
/// The distribution that fits the best to the data we have so far (which is not a lot of data)
/// is from Trowbridge and Reitz's 1975 paper "Average irregularity representation of a rough ray reflection",
/// wildly known as "GGX" (seems to stand for "Ground Glass X" https://twitter.com/CasualEffects/status/783018211130441728).
/// 
/// We will use a generalized version of GGX called Generalized Trowbridge and Reitz (GTR),
/// proposed by Brent Burley and folks at Disney (https://www.disneyanimation.com/publications/physically-based-shading-at-disney/)
/// as our normal distribution function. GTR2 is equivalent to GGX.

/// Schlick's Fresnel equation approximation
/// from "An Inexpensive BRDF Model for Physically-based Rendering", Schlick
/// https://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.50.2297&rep=rep1&type=pdf
/// See "Memo on Fresnel equations" from Sebastien Lagarde
/// for a really nice introduction.
/// https://seblagarde.wordpress.com/2013/04/29/memo-on-fresnel-equations/
template <typename T>
inline T schlick_fresnel(const T &F0, Real cos_theta) {
    return F0 + (Real(1) - F0) *
        pow(max(1 - cos_theta, Real(0)), Real(5));
}

/// Fresnel equation of a dielectric interface.
/// https://seblagarde.wordpress.com/2013/04/29/memo-on-fresnel-equations/
/// n_dot_i: abs(cos(incident angle))
/// n_dot_t: abs(cos(transmission angle))
/// eta: eta_transmission / eta_incident
inline Real fresnel_dielectric(Real n_dot_i, Real n_dot_t, Real eta) {
    assert(n_dot_i >= 0 && n_dot_t >= 0 && eta > 0);
    Real rs = (n_dot_i - eta * n_dot_t) / (n_dot_i + eta * n_dot_t);
    Real rp = (eta * n_dot_i - n_dot_t) / (eta * n_dot_i + n_dot_t);
    Real F = (rs * rs + rp * rp) / 2;
    return F;
}

/// https://seblagarde.wordpress.com/2013/04/29/memo-on-fresnel-equations/
/// This is a specialized version for the code above, only using the incident angle.
/// The transmission angle is derived from 
/// n_dot_i: cos(incident angle) (can be negative)
/// eta: eta_transmission / eta_incident
inline Real fresnel_dielectric(Real n_dot_i, Real eta) {
    //printf("eta is %f \n", eta);
    assert(eta > 0);
    Real n_dot_t_sq = 1 - (1 - n_dot_i * n_dot_i) / (eta * eta);
    if (n_dot_t_sq < 0) {
        // total internal reflection
        return 1;
    }
    Real n_dot_t = sqrt(n_dot_t_sq);
    return fresnel_dielectric(fabs(n_dot_i), n_dot_t, eta);
}

inline Real GTR2(Real n_dot_h, Real roughness) {
    Real alpha = roughness * roughness;
    Real a2 = alpha * alpha;
    Real t = 1 + (a2 - 1) * n_dot_h * n_dot_h;
    return a2 / (c_PI * t*t);
}

inline Real GGX(Real n_dot_h, Real roughness) {
    return GTR2(n_dot_h, roughness);
}

// Modified Fresnel for final uber BSDF metal term
inline Spectrum Fresnel_with_tint(Spectrum base_color, Real specular_tint, Real specular,
    Real eta, Real metallic, Real half_angle) {

    Spectrum C_tint = luminance(base_color) > 0 ? base_color / luminance(base_color) : make_const_spectrum(1);
    Spectrum K_s = (1 - specular_tint) + specular_tint * C_tint;

    Real R_0     = pow(eta - 1, 2) / pow(eta + 1, 2);
    Spectrum C_0 = specular * R_0 * (1 - metallic) * K_s + metallic * base_color;

    Spectrum F_hat_m = C_0 + (1 - C_0) * pow((1 - half_angle), 5);

    return F_hat_m;
}

/// GGX with disney mapping
inline Real Disney_GGX(Real anisotropic, Real roughness, Vector3 h_l) {
    Real aspect = sqrt(Real(1) - Real(0.9) * anisotropic);
    Real a_min = Real(0.0001);
    Real a_x = max(a_min, pow(roughness, 2) / aspect);
    Real a_y = max(a_min, pow(roughness, 2) * aspect);

    Real t = (pow(h_l.x, 2) / pow(a_x, 2)) + (pow(h_l.y, 2) / pow(a_y, 2)) + pow(h_l.z, 2);
    Real D_m = 1 / (c_PI * a_x * a_y * pow(t, 2));

    return D_m;
}

/// Smith with disney mapping
inline Real Disney_Smith(Real anisotropic, Real roughness, Vector3 w, Frame frame) {
    Real aspect = sqrt(Real(1) - Real(0.9) * anisotropic);
    Real a_min = Real(0.0001);
    Real a_x = max(a_min, pow(roughness, 2) / aspect);
    Real a_y = max(a_min, pow(roughness, 2) * aspect);
    Vector3 w_l = to_local(frame, w);

    Real t = (pow(w_l.x * a_x, 2) + pow(w_l.y * a_y, 2)) / pow(w_l.z, 2);

    Real Lamda = (sqrt(Real(1) + t) - Real(1)) / 2;
    Real G_w = Real(1) / (Real(1) + Lamda);

    return G_w;
}

/// Clearcoat GGX
inline Real Clearcoat_GGX(Real gloss, Vector3 h_l) {
    Real a_g = (Real(1) - gloss) * Real(0.1) + gloss * Real(0.001);
    Real a_g_2 = pow(a_g, 2);
    Real D_c = (a_g_2 - Real(1)) / (c_PI * log2(a_g_2) * (1 + (a_g_2 - 1) * (h_l.z)));

    return D_c;
}

/// Clearcoat Smith with disney mapping
inline Real Clearcoat_Smith(Vector3 w, Frame frame) {
    Vector3 w_l = to_local(frame, w);
    Real roughness = Real(0.25);

    Real t = (pow(w_l.x * roughness, 2) + pow(w_l.y * roughness, 2)) / pow(w_l.z, 2);

    Real Lamda = (sqrt(Real(1) + t) - Real(1)) / 2;
    Real G_c = Real(1) / (Real(1) + Lamda);

    return G_c;
}

/// GGX VNDF Sampling Routine
/// https://jcgt.org/published/0007/04/01/paper.pdf
inline Vector3 sampleGGXVNDF(const Vector3& local_dir_in, Real alpha_x, Real alpha_y, const Vector2& rnd_param) {
    // The incoming direction is in the "ellipsodial configuration" in Heitz's paper
    if (local_dir_in.z < 0) {
        // Ensure the input is on top of the surface.
        return -sampleGGXVNDF(-local_dir_in, alpha_x, alpha_y, rnd_param);
    }

    // Transform the incoming direction to the "hemisphere configuration".
    Vector3 hemi_dir_in = normalize(
        Vector3{ alpha_x * local_dir_in.x, alpha_y * local_dir_in.y, local_dir_in.z });

    // Parameterization of the projected area of a hemisphere.
    // First, sample a disk.
    Real r = sqrt(rnd_param.x);
    Real phi = 2 * c_PI * rnd_param.y;
    Real t1 = r * cos(phi);
    Real t2 = r * sin(phi);
    // Vertically scale the position of a sample to account for the projection.
    Real s = (1 + hemi_dir_in.z) / 2;
    t2 = (1 - s) * sqrt(1 - t1 * t1) + s * t2;
    // Point in the disk space
    Vector3 disk_N{ t1, t2, sqrt(max(Real(0), 1 - t1 * t1 - t2 * t2)) };

    // Reprojection onto hemisphere -- we get our sampled normal in hemisphere space.
    Frame hemi_frame(hemi_dir_in);
    Vector3 hemi_N = to_world(hemi_frame, disk_N);

    // Transforming the normal back to the ellipsoid configuration
    return normalize(Vector3{ alpha_x * hemi_N.x, alpha_y * hemi_N.y, max(Real(0), hemi_N.z) });
}

/// The masking term models the occlusion between the small mirrors of the microfacet models.
/// See Eric Heitz's paper "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
/// for a great explanation.
/// https://jcgt.org/published/0003/02/03/paper.pdf
/// The derivation is based on Smith's paper "Geometrical shadowing of a random rough surface".
/// Note that different microfacet distributions have different masking terms.
inline Real smith_masking_gtr2(const Vector3 &v_local, Real roughness) {
    Real alpha = roughness * roughness;
    Real a2 = alpha * alpha;
    Vector3 v2 = v_local * v_local;
    Real Lambda = (-1 + sqrt(1 + (v2.x * a2 + v2.y * a2) / v2.z)) / 2;
    return 1 / (1 + Lambda);
}

/// See "Sampling the GGX Distribution of Visible Normals", Heitz, 2018.
/// https://jcgt.org/published/0007/04/01/
inline Vector3 sample_visible_normals(const Vector3 &local_dir_in, Real alpha, const Vector2 &rnd_param) {
    // The incoming direction is in the "ellipsodial configuration" in Heitz's paper
    if (local_dir_in.z < 0) {
        // Ensure the input is on top of the surface.
        return -sample_visible_normals(-local_dir_in, alpha, rnd_param);
    }

    // Transform the incoming direction to the "hemisphere configuration".
    Vector3 hemi_dir_in = normalize(
        Vector3{alpha * local_dir_in.x, alpha * local_dir_in.y, local_dir_in.z});

    // Parameterization of the projected area of a hemisphere.
    // First, sample a disk.
    Real r = sqrt(rnd_param.x);
    Real phi = 2 * c_PI * rnd_param.y;
    Real t1 = r * cos(phi);
    Real t2 = r * sin(phi);
    // Vertically scale the position of a sample to account for the projection.
    Real s = (1 + hemi_dir_in.z) / 2;
    t2 = (1 - s) * sqrt(1 - t1 * t1) + s * t2;
    // Point in the disk space
    Vector3 disk_N{t1, t2, sqrt(max(Real(0), 1 - t1*t1 - t2*t2))};

    // Reprojection onto hemisphere -- we get our sampled normal in hemisphere space.
    Frame hemi_frame(hemi_dir_in);
    Vector3 hemi_N = to_world(hemi_frame, disk_N);

    // Transforming the normal back to the ellipsoid configuration
    return normalize(Vector3{alpha * hemi_N.x, alpha * hemi_N.y, max(Real(0), hemi_N.z)});
}
