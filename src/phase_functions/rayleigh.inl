#include "../frame.h"

Spectrum eval_op::operator()(const Rayleigh&) const {
    Real rayleigh = 0.75 * (1 + dot(dir_in, dir_out) * dot(dir_in, dir_out));
    return make_const_spectrum(c_INVFOURPI * rayleigh);
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
    return c_INVFOURPI;
}
