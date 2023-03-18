#include "../volume.h"

Spectrum get_majorant_op::operator()(const AtmosphereMedium &m) {
    return make_zero_spectrum();
}

Spectrum get_sigma_s_op::operator()(const AtmosphereMedium &m) {
    // use precalculated result on earth for now
    // need to generalize for other planets
    // https://www.alanzucconi.com/2017/10/10/atmospheric-scattering-3/

    // potentially make it wave based???

    // precomputed scattering coefficients at sea level (density = 1)
    // computed with units in kilometers
    Real my_beta_r = 0.00519673173; //for 680nm 
    Real my_beta_g = 0.01214269792; //for 550nm
    Real my_beta_b = 0.02964525861; //for 440nm

    // betas from "Efficient Rendering of Atmospheric Phenomena"
    Real beta_r = 0.0058; //for 680nm 
    Real beta_g = 0.0135; //for 550nm
    Real beta_b = 0.0331; //for 440nm

                  
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
    // Density function 
    Real roh = exp(-height / scale_height);

    // correction factor for the anisotropic properties of air molecules
    Real anisotropic = 1.06081;

    Real sigma_r = my_beta_r * roh;
    Real sigma_g = my_beta_g * roh;
    Real sigma_b = my_beta_b * roh;

    return Spectrum{ sigma_r, sigma_g, sigma_b };


    //printf("dist is: %.20lf \n", dist);
    //printf("height is: %.20lf \n", height);


    //printf("roh is: %.20lf \n", roh);
    //printf("sigma[0] is: %lf \n", beta_r * roh);
    //return Spectrum{ beta_r * roh, beta_g * roh, beta_b * roh};
}

Spectrum get_sigma_a_op::operator()(const AtmosphereMedium &m) {
    // Assume no absorbtion in atmosphere
    return make_zero_spectrum();
}
