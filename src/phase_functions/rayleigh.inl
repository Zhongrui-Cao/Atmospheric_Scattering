#include "../frame.h"

//TODO change file name

Spectrum eval_op::operator()(const Rayleigh&) const {
    Real mu = -dot(dir_in, dir_out);

    // rayleigh
    Spectrum rayleigh = make_const_spectrum(c_INVFOURPI * 0.75 * (Real(1) + mu * mu));

    // mie
    // The asymetry parameter for the Cornette-Shanks phase function for the aerosols.
    Real g = 0.76;
    Real top = (1.0 - g*g) * (1.0 + mu*mu);
    Real bottom = (2.0 + g*g) * sqrt(pow(1.0+g*g-2.0*g*mu, 3.0));
    Spectrum mie = make_const_spectrum(c_INVFOURPI * 1.5 * top / bottom);

    Spectrum blend = 0.5 * rayleigh + 0.5 * mie;

    return blend;
}

std::optional<Vector3> sample_phase_function_op::operator()(const Rayleigh&) const {
    // TODO
    // https://backend.orbit.dtu.dk/ws/portalfiles/portal/6314521/3D9A1d01.pdf

    // Uniform sphere sampling
    Real z = 1 - 2 * rnd_param.x;
    Real r = sqrt(fmax(Real(0), 1 - z * z));
    Real phi = 2 * c_PI * rnd_param.y;
    return Vector3{ r * cos(phi), r * sin(phi), z };
}

Real pdf_sample_phase_op::operator()(const Rayleigh&) const {
    // TODO
    // https://backend.orbit.dtu.dk/ws/portalfiles/portal/6314521/3D9A1d01.pdf

    return c_INVFOURPI;
}
