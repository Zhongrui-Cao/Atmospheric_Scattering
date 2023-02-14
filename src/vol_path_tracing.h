#pragma once

// The simplest volumetric renderer: 
// single absorption only homogeneous volume
// only handle directly visible light sources
Spectrum vol_path_tracing_1(const Scene &scene,
                            int x, int y, /* pixel coordinates */
                            pcg32_state &rng) {
    // Homework 2: implememt this!
    int w = scene.camera.width, h = scene.camera.height;
    Vector2 screen_pos((x + next_pcg32_real<Real>(rng)) / w,
                       (y + next_pcg32_real<Real>(rng)) / h);
    Ray ray = sample_primary(scene.camera, screen_pos);

    RayDifferential ray_diff = RayDifferential{Real(0), Real(0)};
    std::optional<PathVertex> vertex_ = intersect(scene, ray, ray_diff);

    if (vertex_) {
        PathVertex vertex = *vertex_;
        Medium ext_medium = scene.media[vertex.exterior_medium_id];
        Spectrum sigma_a = get_sigma_a(ext_medium, vertex.position);
        Real t = distance(ray.org, vertex.position);
        Spectrum transmittance = exp(-sigma_a * t);
        Spectrum Le = make_zero_spectrum();
        if (is_light(scene.shapes[vertex.shape_id])){
            Le = emission(vertex, -ray.dir, scene);
        }
        return transmittance * Le;
    }
    return make_zero_spectrum();
}

// vol_path_tracing_2 helper
Spectrum L_s1(const Scene& scene, 
                    const Ray& ray,
                    const Medium& medium,
                    const Spectrum& p, 
                    const PointAndNormal& pan,
                    const Light& light,
                    const int light_id){
    
    // vars needed //
    // dir in, aka ray dir
    Spectrum w = -ray.dir;
    // dir out, aka p to light dir
    Spectrum w_prime = normalize(pan.position - p);
    // point on light
    Spectrum p_prime = pan.position;
    // normal on light
    Spectrum n_p_prime = pan.normal;

    // phase function
    PhaseFunction pf = get_phase_function(medium);
    Spectrum ro = eval(pf, w, w_prime);

    // emission from light
    Spectrum Le = emission(light, -w_prime, Real(0), pan, scene);

    // exponential term
    Spectrum sigma_t = get_sigma_a(medium, p) + get_sigma_s(medium, p);
    Spectrum expterm = exp(-sigma_t * length(p - p_prime));

    // Geometry term
    Real geom = abs(-dot(w_prime, n_p_prime)) / distance_squared(p, p_prime);

    // shoot a shadow ray to get visibility
    Ray shadow_ray{p, w_prime, 
                get_shadow_epsilon(scene),
                (1 - get_shadow_epsilon(scene)) *
                    distance(p_prime, p)};

    Real visible = !occluded(scene, shadow_ray);

    // put together
    Spectrum L_s1_estimate = ro * Le * expterm * geom * visible;

    // importance sample the light
    Real pdf = pdf_point_on_light(light, pan, p, scene) * light_pmf(scene, light_id);

    return L_s1_estimate / pdf;
}

// The second simplest volumetric renderer: 
// single monochromatic homogeneous volume with single scattering,
// no need to handle surface lighting, only directly visible light source
Spectrum vol_path_tracing_2(const Scene &scene,
                            int x, int y, /* pixel coordinates */
                            pcg32_state &rng) {
    // Homework 2: implememt this!
    int w = scene.camera.width, h = scene.camera.height;
    Vector2 screen_pos((x + next_pcg32_real<Real>(rng)) / w,
                       (y + next_pcg32_real<Real>(rng)) / h);
    Ray ray = sample_primary(scene.camera, screen_pos);

    RayDifferential ray_diff = RayDifferential{Real(0), Real(0)};
    std::optional<PathVertex> vertex_ = intersect(scene, ray, ray_diff);

    if (vertex_) {
        PathVertex vertex = *vertex_;
        Real u = next_pcg32_real<Real>(rng);
        Medium ext_medium = scene.media[vertex.exterior_medium_id];
        Real sigma_a = get_sigma_a(ext_medium, vertex.position).x;
        Real sigma_s = get_sigma_s(ext_medium, vertex.position).x;
        Real sigma_t = sigma_a + sigma_s;

        Real t = - log(1.0 - u) / sigma_t;
        Real t_hit = distance(ray.org, vertex.position);

        if (t < t_hit) {
            Real trans_pdf = exp(-sigma_t * t) * sigma_t;
            Real transmittance = exp(-sigma_t * t);
            Spectrum p = ray.org + t * ray.dir;
            int light_id = sample_light(scene, next_pcg32_real<Real>(rng));
            Light light = scene.lights[light_id];
            PointAndNormal pan = sample_point_on_light(light, p, 
                                Vector2(next_pcg32_real<Real>(rng), next_pcg32_real<Real>(rng)), 
                                next_pcg32_real<Real>(rng), scene);
            Spectrum ret =  L_s1(scene, ray, ext_medium, p, pan, light, light_id);
            return (transmittance / trans_pdf) * sigma_s * ret;
        }
        else {
            Real trans_pdf = exp(-sigma_t * t_hit);
            Real transmittance = exp(-sigma_t * t_hit);
            Spectrum Le = make_zero_spectrum();
            if (is_light(scene.shapes[vertex.shape_id])){
                Le = emission(vertex, -ray.dir, scene);
            }
            return (transmittance / trans_pdf) * Le;
        }
    } 
    else {
        Real u = next_pcg32_real<Real>(rng);
        Medium ext_medium = scene.media[scene.camera.medium_id];
        Real sigma_a = get_sigma_a(ext_medium, Vector3(1,2,3)).x;
        Real sigma_s = get_sigma_s(ext_medium, Vector3(4,5,6)).x;
        Real sigma_t = sigma_a + sigma_s;

        Real t = - log(1.0 - u) / sigma_t;

        Real trans_pdf = exp(-sigma_t * t) * sigma_t;
        Real transmittance = exp(-sigma_t * t);
        Spectrum p = ray.org + t * ray.dir;
        int light_id = sample_light(scene, next_pcg32_real<Real>(rng));
        Light light = scene.lights[light_id];
        PointAndNormal pan = sample_point_on_light(light, p, 
                            Vector2(next_pcg32_real<Real>(rng), next_pcg32_real<Real>(rng)), 
                            next_pcg32_real<Real>(rng), scene);
        Spectrum ret =  L_s1(scene, ray, ext_medium, p, pan, light, light_id);
        return (transmittance / trans_pdf) * sigma_s * ret;
    }

}

// vol_3 helper
int update_medium(PathVertex isect, Ray ray){

}

// The third volumetric renderer (not so simple anymore): 
// multiple monochromatic homogeneous volumes with multiple scattering
// no need to handle surface lighting, only directly visible light source
Spectrum vol_path_tracing_3(const Scene &scene,
                            int x, int y, /* pixel coordinates */
                            pcg32_state &rng) {
    // Homework 2: implememt this!

    int w = scene.camera.width, h = scene.camera.height;
    Vector2 screen_pos((x + next_pcg32_real<Real>(rng)) / w,
                       (y + next_pcg32_real<Real>(rng)) / h);
    Ray ray = sample_primary(scene.camera, screen_pos);
    RayDifferential ray_diff = RayDifferential{Real(0), Real(0)};

    int current_medium = scene.camera.medium_id;
    Real current_path_throughput = 1;
    Spectrum radiance = make_zero_spectrum();
    int bounces = 0;
    int max_depth = scene.options.max_depth;

    while(true) {
        bool scatter = false;
        std::optional<PathVertex> vertex_ = intersect(scene, ray, ray_diff);
        Real transmittance = 1;
        Real trans_pdf = 1;
        if (current_medium) {
            Real u = next_pcg32_real<Real>(rng);
            Medium medium = scene.media[current_medium];
            Real sigma_a = get_sigma_a(medium, Vector3(1,2,3)).x;
            Real sigma_s = get_sigma_s(medium, Vector3(1,2,3)).x;
            Real sigma_t = sigma_a + sigma_s;
            Real t = - log(1.0 - u) / sigma_t;

            //ray hit
            if (vertex_) {
                PathVertex vertex = *vertex_;
                Real t_hit = distance(ray.org, vertex.position);
                // if t not on surface
                if (t < t_hit) {
                    scatter = true;
                    trans_pdf = exp(-sigma_t * t) * sigma_t;
                    transmittance = exp(-sigma_t * t);
                } else {
                    scatter = false;
                    trans_pdf = exp(-sigma_t * t_hit);
                    transmittance = exp(-sigma_t * t_hit);
                }
            } else {
                scatter = true;
                trans_pdf = exp(-sigma_t * t) * sigma_t;
                transmittance = exp(-sigma_t * t);
            }
            ray.org = ray.org + t * ray.dir;
        }
        current_path_throughput *= (transmittance / trans_pdf);

        if (!scatter) {
            PathVertex vertex = *vertex_;
            radiance += current_path_throughput * emission(vertex, -ray.dir, scene);
        }

        if (bounces == max_depth && max_depth != -1) {
            break;
        }

        if (!scatter && vertex_) {
            PathVertex vertex = *vertex_;
            // if index matching surface
            if (vertex.material_id == -1) {
                current_medium = update_medium(vertex, ray);
                bounces += 1;
                continue;
            }
        }

        if (scatter) {
            
        }
    }

    




    return make_zero_spectrum();
}

// The fourth volumetric renderer: 
// multiple monochromatic homogeneous volumes with multiple scattering
// with MIS between next event estimation and phase function sampling
// still no surface lighting
Spectrum vol_path_tracing_4(const Scene &scene,
                            int x, int y, /* pixel coordinates */
                            pcg32_state &rng) {
    // Homework 2: implememt this!
    return make_zero_spectrum();
}

// The fifth volumetric renderer: 
// multiple monochromatic homogeneous volumes with multiple scattering
// with MIS between next event estimation and phase function sampling
// with surface lighting
Spectrum vol_path_tracing_5(const Scene &scene,
                            int x, int y, /* pixel coordinates */
                            pcg32_state &rng) {
    // Homework 2: implememt this!
    return make_zero_spectrum();
}

// The final volumetric renderer: 
// multiple chromatic heterogeneous volumes with multiple scattering
// with MIS between next event estimation and phase function sampling
// with surface lighting
Spectrum vol_path_tracing(const Scene &scene,
                          int x, int y, /* pixel coordinates */
                          pcg32_state &rng) {
    // Homework 2: implememt this!
    return make_zero_spectrum();
}
