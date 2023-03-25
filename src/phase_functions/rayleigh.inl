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
    
    // Uniform sphere sampling
    // Real z = 1 - 2 * rnd_param.x;
    // Real r = sqrt(fmax(Real(0), 1 - z * z));
    // Real phi = 2 * c_PI * rnd_param.y;
    // Vector3 rayleigh_sample = Vector3{ r * cos(phi), r * sin(phi), z };

    // 50/50 determine sample rayleigh or Mie
    if (rnd_num > 0.5) {

        // inverse transform sample the rayleigh function
        // https://backend.orbit.dtu.dk/ws/portalfiles/portal/6314521/3D9A1d01.pdf
        Real term = 2 * rnd_param.x - 1;
        Real term1 = 2 * term;
        Real term2 = sqrt(4 * term * term + 1);
        Real u = -pow(term1 + term2, Real(1) / Real(3));
        Real cos_elevation = u - 1 / u;
        Real sin_elevation = sqrt(max(1 - cos_elevation * cos_elevation, Real(0)));
        Real azimuth = 2 * c_PI * rnd_param.y;
        Frame frame(dir_in);
        return to_world(frame,
            Vector3{ sin_elevation * cos(azimuth),
                    sin_elevation * sin(azimuth),
                    cos_elevation });
    }
    else {
        // HG sampling, since very similar to CS
        // https://www.shadertoy.com/view/wlGBzh
        Real g = 0.76;
        Real tmp = (g * g - 1) / (2 * rnd_param.x * g - (g + 1));
        Real cos_elevation = (tmp * tmp - (1 + g * g)) / (2 * g);
        Real sin_elevation = sqrt(max(1 - cos_elevation * cos_elevation, Real(0)));
        Real azimuth = 2 * c_PI * rnd_param.y;
        Frame frame(dir_in);
        return to_world(frame,
            Vector3{ sin_elevation * cos(azimuth),
                    sin_elevation * sin(azimuth),
                    cos_elevation });
    }

}

Real pdf_sample_phase_op::operator()(const Rayleigh&) const {

    // rayleigh
    // Real rayleigh_pdf = c_INVFOURPI;
    Real mu = -dot(dir_in, dir_out);
    Real rayleigh_pdf = c_INVFOURPI * 0.75 * (Real(1) + mu * mu);

    Real g = 0.76;
    Real HG_pdf = c_INVFOURPI *
        (1 - g * g) / 
            (pow((1 + g * g + 2 * g * dot(dir_in, dir_out)), Real(3)/Real(2)));

    return 0.5 * HG_pdf + 0.5 * rayleigh_pdf;
}
