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
    // unit in meters
    Real beta_r = 0.0000051967; //680nm
    Real beta_g = 0.0000121427; //550nm
    Real beta_b = 0.0000296453; //440nm

    // Get height of curr ray
    // Assume earth center is at 0, 0, 0. Using radius of earth + atmosphere = 1.
    Real earth_radius = 0.98456313677;
    //Real height = (distance(ray_org, Vector3{0, 0, 0}) - earth_radius);
    Real height = (distance(p, Vector3{ 0, 0, 0 }) - earth_radius);
    
    // convert to km
    height = height * 6478;
    //printf("height is: %.20lf \n", height);

    // Density function
    Real scale_height = 8500;
    Real roh = exp(- height / scale_height);

    //printf("roh is: %.20lf \n", roh);
    //printf("sigma[0] is: %lf \n", beta_r * roh);
    //return Spectrum{ beta_r * roh, beta_g * roh, beta_b * roh};
    return Spectrum{ beta_r * 10000, beta_g * 10000, beta_b * 10000};
}

Spectrum get_sigma_a_op::operator()(const AtmosphereMedium &m) {
    return make_zero_spectrum();
}
