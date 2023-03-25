#include "../volume.h"

Spectrum get_majorant_op::operator()(const AtmosphereMedium &m) {
    return make_zero_spectrum();
}

Spectrum get_sigma_s_op::operator()(const AtmosphereMedium &m) {
    // use precalculated result on earth for now
    // need to generalize for other planets
    // https://www.alanzucconi.com/2017/10/10/atmospheric-scattering-3/

    // potentially make it wave based???

    ////////////////////////////Rayleigh starts here//////////////////////////////////////////
    // precomputed scattering coefficients at sea level (density = 1)
    // computed with units in kilometers
    Real my_beta_r = 0.00519673173; //for 680nm 
    Real my_beta_g = 0.01214269792; //for 550nm
    Real my_beta_b = 0.02964525861; //for 440nm

    // betas from "Efficient Rendering of Atmospheric Phenomena"
    //Real beta_r = 0.0058; //for 680nm 
    //Real beta_g = 0.0135; //for 550nm
    //Real beta_b = 0.0331; //for 440nm

                  
    // Get height of curr ray
    // Assume earth center is at 0, 0, 0.
    Real earth_radius = 6378;
    Real dist = distance(p, Vector3{ 0, 0, 0 });

    // reject if inside earth or in outerspace
    if (dist > 6438 || dist < 6378) {
        return make_zero_spectrum();
    }

    // height in km
    Real height = (dist - earth_radius);

    // scale_height: thickness of the atmosphere if its density were uniform.
    Real scale_height = 8.5;
    Real scale_height_paper = 8.0;
    // Density function 
    Real roh = exp(-height / scale_height);
    Real roh_paper = exp(-height / scale_height_paper);

    // correction factor for the anisotropic properties of air molecules
    Real anisotropic = 1.06081;

    Real sigma_r = my_beta_r * roh;
    Real sigma_g = my_beta_g * roh;
    Real sigma_b = my_beta_b * roh;

    //Real sigma_r_paper = beta_r * roh_paper;
    //Real sigma_g_paper = beta_g * roh_paper;
    //Real sigma_b_paper = beta_b * roh_paper;

    Spectrum rayleigh_sigma = Spectrum{ sigma_r, sigma_g, sigma_b };
    //Spectrum rayleigh_sigma_paper = Spectrum{ sigma_r_paper, sigma_g_paper, sigma_b_paper };


    ////////////////////////////Mie starts here//////////////////////////////////////////
    // Since Mie too complicated we approximate with Cornette-Shanks 
    // numbers from Precomputed Atmospheric Scattering

    // beta mesured at sea level in km^-1
    Real beta_sealevel = 0.0000021;
    
    // scale height for mie
    Real H_m = 1.2;
    // density function
    Real roh_mie = exp(-height/H_m);
    Real sigma_s_mie = beta_sealevel * roh_mie;
    Spectrum mie_sigma = make_const_spectrum(sigma_s_mie);

    //Spectrum blend = rayleigh_sigma + mie_sigma;
    //Spectrum blend_paper = rayleigh_sigma_paper + mie_sigma;

    return rayleigh_sigma + mie_sigma;
}

Spectrum get_sigma_a_op::operator()(const AtmosphereMedium &m) {
    // get mie sigma a

    // Get height of curr ray
    // Assume earth center is at 0, 0, 0.
    Real earth_radius = 6378;
    Real dist = distance(p, Vector3{ 0, 0, 0 });

    // reject if inside earth or in outerspace
    if (dist > 6438 || dist < 6378) {
        return make_zero_spectrum();
    }

    // height in km
    Real height = (dist - earth_radius);

    // turbidity - how muddy/not clear is the sky
    // absorption is scattering/turbidity
    Real turb = 9;

    // TODO: not efficient, double calculation
    // beta mesured at sea level in km^-1
    Real beta_sealevel = 0.0000021;
    // scale height for mie
    Real H_m = 1.2;
    // density function
    Real roh_mie = exp(-height/H_m);
    Real sigma_s_mie = beta_sealevel * roh_mie;
    Spectrum mie_sigma_a = make_const_spectrum(sigma_s_mie / turb);

    // assume no absorption from rayleigh
    // return mie_sigma_a * 0.5;
    return mie_sigma_a;
    
}
