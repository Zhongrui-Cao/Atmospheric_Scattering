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

    int medium = 0;
    if (isect.interior_medium_id != isect.exterior_medium_id) {

        if (dot(ray.dir, isect.geometric_normal) > 0) {
            medium = isect.exterior_medium_id;
        }
        else {
            medium = isect.interior_medium_id;
        }
    }
    return medium;
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
    Spectrum current_path_throughput = make_const_spectrum(1);
    Spectrum radiance = make_zero_spectrum();
    int bounces = 0;
    int max_depth = scene.options.max_depth;

    while(true) {
        bool scatter = false;
        std::optional<PathVertex> vertex_ = intersect(scene, ray, ray_diff);
        Real transmittance = 1;
        Real trans_pdf = 1;
        Real t = 0;
        if (current_medium != -1) {
            Real u = next_pcg32_real<Real>(rng);
            Medium medium = scene.media[current_medium];
            Real sigma_a = get_sigma_a(medium, Vector3(1,2,3)).x;
            Real sigma_s = get_sigma_s(medium, Vector3(1,2,3)).x;
            Real sigma_t = sigma_a + sigma_s;
            t = - log(1.0 - u) / sigma_t;

            //ray hit
            if (vertex_) {
                PathVertex vertex = *vertex_;
                Real t_hit = distance(ray.org, vertex.position);
                // if t not on surface
                if (t < t_hit) {
                    scatter = true;
                    trans_pdf = exp(-sigma_t * t) * sigma_t;
                    transmittance = exp(-sigma_t * t);
                    ray.org = ray.org + t * ray.dir;
                } else {
                    scatter = false;
                    trans_pdf = exp(-sigma_t * t_hit);
                    transmittance = exp(-sigma_t * t_hit);
                    ray.org = ray.org + t_hit * ray.dir;
                }
            } else {
                scatter = true;
                trans_pdf = exp(-sigma_t * t) * sigma_t;
                transmittance = exp(-sigma_t * t);
                ray.org = ray.org + t * ray.dir;
            }
            
        }
        current_path_throughput *= (transmittance / trans_pdf);

        if (!scatter && vertex_) {
            PathVertex vertex = *vertex_;

            if (is_light(scene.shapes[vertex.shape_id])){
                radiance += current_path_throughput * emission(vertex, -ray.dir, scene);
            }
        }

        if (bounces == max_depth - 1 && max_depth != -1) {
            break;
        }

        if (!scatter && vertex_) {
            PathVertex vertex = *vertex_;
            // if index matching surface
            if (vertex.material_id == -1) {
                current_medium = update_medium(vertex, ray);
                bounces += 1;
                ray.org = vertex.position + ray.dir * get_intersection_epsilon(scene);
                continue;
            }
        }

        if (scatter) {
            Vector3 p = ray.org;
            PhaseFunction pf = get_phase_function(scene.media[current_medium]);
            std::optional<Vector3> next_dir_ = sample_phase_function(pf, -ray.dir, Vector2(next_pcg32_real<Real>(rng), next_pcg32_real<Real>(rng)));
            if (next_dir_) {
                Vector3 next_dir = *next_dir_;
                Real sigma_s = get_sigma_s(scene.media[current_medium], p).x;
                current_path_throughput *= (eval(pf, -ray.dir, next_dir) / pdf_sample_phase(pf, -ray.dir, next_dir)) * sigma_s;
                ray = Ray{ray.org + next_dir * get_intersection_epsilon(scene), next_dir, Real(0), infinity<Real>()};
            }
        }
        else {
            break;
        }

        Real rr_prob = 1;

        if (bounces >= max_depth){
            rr_prob = min(current_path_throughput.x, 0.95);
            if (next_pcg32_real<Real>(rng) > rr_prob) {
                break;
            }
            else {
                current_path_throughput /= rr_prob;
            }
        }
        bounces ++;
    }

    return radiance;
}

Spectrum nee(Vector3 p, Vector3 omega, int current_medium, int bounces, Light light, Scene scene, pcg32_state& rng, int light_id) {

    PointAndNormal p_prime = sample_point_on_light(light, p, 
                                Vector2(next_pcg32_real<Real>(rng), next_pcg32_real<Real>(rng)),
                                next_pcg32_real<Real>(rng), scene);

    Real T_light = 1;
    int shadow_medium = current_medium;
    int shadow_bounces = 0;
    Real p_trans_dir = 1;

    Vector3 orig_p = p;

    while (1) {

        Ray shadow_ray = Ray{p, normalize(p_prime.position - p),  get_shadow_epsilon(scene),
                               (1 - get_shadow_epsilon(scene)) *
                                   distance(p_prime.position, p)};
        RayDifferential ray_diff = RayDifferential{ Real(0), Real(0) };
        std::optional<PathVertex> isect = intersect(scene, shadow_ray, ray_diff);
        Real next_t = distance(p, p_prime.position);

        if(isect) {
            PathVertex vertex = *isect;
            next_t = distance(p, vertex.position);
        }
        if(shadow_medium != -1) {
            Medium medium = scene.media[shadow_medium];
            Real sigma_a = get_sigma_a(medium, Vector3(1, 2, 3)).x;
            Real sigma_s = get_sigma_s(medium, Vector3(1, 2, 3)).x;
            Real sigma_t = sigma_a + sigma_s;
            T_light *= exp(-sigma_t * next_t);
            p_trans_dir *= exp(-sigma_t * next_t);
        }
            
        if(!isect) {
            break;
        } 
        else {
            PathVertex vertex = *isect;
            if (vertex.material_id >= 0) {
                return make_zero_spectrum();
            }
            
            shadow_bounces ++;
            int max_bounce = scene.options.max_depth;
            
            if (max_bounce != -1 && bounces + shadow_bounces + 1 >= max_bounce) {
                return make_zero_spectrum();
            }
            shadow_medium = update_medium(vertex, shadow_ray);
            p = p + next_t * (shadow_ray.dir);
        }
    }

    if (T_light > 0) {
        Vector3 omega_prime = normalize(orig_p - p_prime.position);
        Real denom = distance_squared(orig_p, p_prime.position);
        Real top = abs(dot(omega_prime, p_prime.normal));
        Real G = top / denom;
        PhaseFunction pf = get_phase_function(scene.media[current_medium]);

        Spectrum f = eval(pf, omega, -omega_prime);
        Spectrum Le = emission(light, omega_prime, Real(0), p_prime, scene);
        Real pdf_nee = light_pmf(scene, light_id) *
            pdf_point_on_light(light, p_prime, orig_p, scene);
        
        Spectrum contrib = T_light * G * f * Le / pdf_nee;
        Real pdf_phase = pdf_sample_phase(pf, omega, -omega_prime) * G * p_trans_dir;
        Real w = (pdf_nee * pdf_nee) / (pdf_nee * pdf_nee + pdf_phase * pdf_phase);

        return w * contrib;
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
    int w = scene.camera.width, h = scene.camera.height;
    Vector2 screen_pos((x + next_pcg32_real<Real>(rng)) / w,
        (y + next_pcg32_real<Real>(rng)) / h);
    Ray ray = sample_primary(scene.camera, screen_pos);
    RayDifferential ray_diff = RayDifferential{ Real(0), Real(0) };

    int current_medium = scene.camera.medium_id;
    Spectrum current_path_throughput = make_const_spectrum(1);
    Spectrum radiance = make_zero_spectrum();
    int bounces = 0;
    int max_depth = scene.options.max_depth;

    bool never_scatter = true;
    Real dir_pdf = 0;
    Vector3 nee_p_cache;
    Real multi_trans_pdf = 1;

    while (true) {
        bool scatter = false;
        std::optional<PathVertex> vertex_ = intersect(scene, ray, ray_diff);
        Real transmittance = 1;
        Real trans_pdf = 1;
        Real t = 0;

        if (current_medium == -1) {
            if (vertex_) {
                PathVertex vertex = *vertex_;
                ray.org = ray.org + distance(ray.org, vertex.position) * ray.dir;
            } else {
                break;
            }
        }

        if (current_medium != -1) {
            Real u = next_pcg32_real<Real>(rng);
            Medium medium = scene.media[current_medium];
            Real sigma_a = get_sigma_a(medium, Vector3(1, 2, 3)).x;
            Real sigma_s = get_sigma_s(medium, Vector3(1, 2, 3)).x;
            Real sigma_t = sigma_a + sigma_s;
            t = -log(1.0 - u) / sigma_t;

            //ray hit
            if (vertex_) {
                PathVertex vertex = *vertex_;
                Real t_hit = distance(ray.org, vertex.position);
                // if t not on surface
                if (t < t_hit) {
                    scatter = true;
                    trans_pdf = exp(-sigma_t * t) * sigma_t;
                    transmittance = exp(-sigma_t * t);
                    ray.org = ray.org + t * ray.dir;
                }
                else {
                    scatter = false;
                    trans_pdf = exp(-sigma_t * t_hit);
                    transmittance = exp(-sigma_t * t_hit);
                    ray.org = ray.org + t_hit * ray.dir;
                }
            }
            else {
                scatter = true;
                trans_pdf = exp(-sigma_t * t) * sigma_t;
                transmittance = exp(-sigma_t * t);
                ray.org = ray.org + t * ray.dir;
            }
        }
        current_path_throughput *= (transmittance / trans_pdf);

        if (!scatter && vertex_) {
            PathVertex vertex = *vertex_;
            if (never_scatter) {
                if (is_light(scene.shapes[vertex.shape_id])) {
                    radiance += current_path_throughput * emission(vertex, -ray.dir, scene);
                }
            } else {
                if (is_light(scene.shapes[vertex.shape_id])) {
                    PointAndNormal light_point = {vertex.position, vertex.geometric_normal};
                    int light_id = get_area_light_id(scene.shapes[vertex.shape_id]);
                    Light currlight = scene.lights[light_id];
                    Real pdf_nee = pdf_point_on_light(currlight, light_point, nee_p_cache, scene) * light_pmf(scene, light_id);
                    Vector3 omega_prime = normalize(vertex.position - nee_p_cache);
                    Real top = abs(dot(omega_prime, vertex.geometric_normal));
                    Real bottom = length_squared(vertex.position - nee_p_cache);
                    
                    Real G = top/bottom;
                    Real dir_pdf_ = dir_pdf * multi_trans_pdf * G;
                    
                    Real w = (dir_pdf_ * dir_pdf_) / (dir_pdf_ * dir_pdf_ + pdf_nee * pdf_nee);
                    
                    radiance += current_path_throughput * emission(vertex, -ray.dir, scene) * w;
                }
            }
        }

        if (bounces == max_depth - 1 && max_depth != -1) {
            break;
        }

        if (!scatter && vertex_) {
            PathVertex vertex = *vertex_;
            // if index matching surface
            if (vertex.material_id == -1) {
                current_medium = update_medium(vertex, ray);
                bounces += 1;
                ray.org = vertex.position + ray.dir * get_intersection_epsilon(scene);
                multi_trans_pdf *= trans_pdf;
                continue;
            }
        }

        if (scatter) {
            never_scatter = false;
            Vector3 p = ray.org;
            PhaseFunction pf = get_phase_function(scene.media[current_medium]);
            std::optional<Vector3> next_dir_ = sample_phase_function(pf, -ray.dir, Vector2(next_pcg32_real<Real>(rng), next_pcg32_real<Real>(rng)));
            if (next_dir_) {
                Vector3 next_dir = *next_dir_;
                Real sigma_s = get_sigma_s(scene.media[current_medium], p).x;

                int light_id = sample_light(scene, next_pcg32_real<Real>(rng));
                Spectrum nee_out = nee(p, -ray.dir, current_medium, bounces, scene.lights[light_id], scene, rng, light_id);

                radiance += current_path_throughput * nee_out * sigma_s;

                dir_pdf = pdf_sample_phase(pf, -ray.dir, next_dir);
                current_path_throughput *= (eval(pf, -ray.dir, next_dir) / dir_pdf) * sigma_s;

                ray = Ray{ ray.org + next_dir * get_intersection_epsilon(scene), next_dir, Real(0), infinity<Real>() };
                nee_p_cache = p;
                multi_trans_pdf = Real(1);
            }
        }
        else {
            break;
        }

        Real rr_prob = 1;

        if (bounces >= max_depth) {
            rr_prob = min(current_path_throughput.x, 0.95);
            if (next_pcg32_real<Real>(rng) > rr_prob) {
                break;
            }
            else {
                current_path_throughput /= rr_prob;
            }
        }
        bounces++;
    }

    return radiance;
}

Spectrum nee_brdf(Vector3 p, Vector3 omega, int current_medium, int bounces, Light light, Scene scene, pcg32_state& rng, int light_id, PathVertex v) {

    PointAndNormal p_prime = sample_point_on_light(light, p, 
                                Vector2(next_pcg32_real<Real>(rng), next_pcg32_real<Real>(rng)),
                                next_pcg32_real<Real>(rng), scene);

    Real T_light = 1;
    int shadow_medium = current_medium;
    int shadow_bounces = 0;
    Real p_trans_dir = 1;

    Vector3 orig_p = p;

    while (1) {

        Ray shadow_ray = Ray{p, normalize(p_prime.position - p),  get_shadow_epsilon(scene),
                               (1 - get_shadow_epsilon(scene)) *
                                   distance(p_prime.position, p)};
        RayDifferential ray_diff = RayDifferential{ Real(0), Real(0) };
        std::optional<PathVertex> isect = intersect(scene, shadow_ray, ray_diff);
        Real next_t = distance(p, p_prime.position);

        if(isect) {
            PathVertex vertex = *isect;
            next_t = distance(p, vertex.position);
        }
        if(shadow_medium != -1) {
            Medium medium = scene.media[shadow_medium];
            Real sigma_a = get_sigma_a(medium, Vector3(1, 2, 3)).x;
            Real sigma_s = get_sigma_s(medium, Vector3(1, 2, 3)).x;
            Real sigma_t = sigma_a + sigma_s;
            T_light *= exp(-sigma_t * next_t);
            p_trans_dir *= exp(-sigma_t * next_t);
        }
            
        if(!isect) {
            break;
        } 
        else {
            PathVertex vertex = *isect;
            if (vertex.material_id >= 0) {
                return make_zero_spectrum();
            }
            
            shadow_bounces ++;
            int max_bounce = scene.options.max_depth;
            
            if (max_bounce != -1 && bounces + shadow_bounces + 1 >= max_bounce) {
                return make_zero_spectrum();
            }
            shadow_medium = update_medium(vertex, shadow_ray);
            p = p + next_t * (shadow_ray.dir);
        }
    }

    if (T_light > 0) {
        Vector3 omega_prime = normalize(orig_p - p_prime.position);
        Real denom = distance_squared(orig_p, p_prime.position);
        Real top = abs(dot(omega_prime, p_prime.normal));
        Real G = top / denom;

        const Material& mat = scene.materials[v.material_id];

        Spectrum f = eval(mat, omega, -omega_prime, v, scene.texture_pool);
        Spectrum Le = emission(light, omega_prime, Real(0), p_prime, scene);
        Real pdf_nee = light_pmf(scene, light_id) *
            pdf_point_on_light(light, p_prime, orig_p, scene);
        
        Spectrum contrib = T_light * G * f * Le / pdf_nee;
        Real pdf_bsdf = pdf_sample_bsdf(mat, omega, -omega_prime, v, scene.texture_pool) * G * p_trans_dir;
        Real w = (pdf_nee * pdf_nee) / (pdf_nee * pdf_nee + pdf_bsdf * pdf_bsdf);

        return w * contrib;
    }

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
    int w = scene.camera.width, h = scene.camera.height;
    Vector2 screen_pos((x + next_pcg32_real<Real>(rng)) / w,
        (y + next_pcg32_real<Real>(rng)) / h);
    Ray ray = sample_primary(scene.camera, screen_pos);
    RayDifferential ray_diff = RayDifferential{ Real(0), Real(0) };

    int current_medium = scene.camera.medium_id;
    Spectrum current_path_throughput = make_const_spectrum(1);
    Spectrum radiance = make_zero_spectrum();
    int bounces = 0;
    int max_depth = scene.options.max_depth;

    bool never_scatter = true;
    Real dir_pdf = 0;
    Vector3 nee_p_cache;
    Real multi_trans_pdf = 1;

    while (true) {
        bool scatter = false;
        std::optional<PathVertex> vertex_ = intersect(scene, ray, ray_diff);
        Real transmittance = 1;
        Real trans_pdf = 1;
        Real t = 0;

        if (current_medium == -1) {
            if (vertex_) {
                PathVertex vertex = *vertex_;
                ray.org = ray.org + distance(ray.org, vertex.position) * ray.dir;
            } else {
                break;
            }
        }

        if (current_medium != -1) {
            Real u = next_pcg32_real<Real>(rng);
            Medium medium = scene.media[current_medium];
            Real sigma_a = get_sigma_a(medium, Vector3(1, 2, 3)).x;
            Real sigma_s = get_sigma_s(medium, Vector3(1, 2, 3)).x;
            Real sigma_t = sigma_a + sigma_s;
            t = -log(1.0 - u) / sigma_t;
            //ray hit
            if (vertex_) {
                PathVertex vertex = *vertex_;
                Real t_hit = distance(ray.org, vertex.position);
                // if t not on surface
                if (t < t_hit) {
                    scatter = true;
                    trans_pdf = exp(-sigma_t * t) * sigma_t;
                    transmittance = exp(-sigma_t * t);
                    ray.org = ray.org + t * ray.dir;
                }
                else {
                    scatter = false;
                    trans_pdf = exp(-sigma_t * t_hit);
                    transmittance = exp(-sigma_t * t_hit);
                    ray.org = ray.org + t_hit * ray.dir;
                }
            }
            else {
                scatter = true;
                trans_pdf = exp(-sigma_t * t) * sigma_t;
                transmittance = exp(-sigma_t * t);
                ray.org = ray.org + t * ray.dir;
            }
        }
        current_path_throughput *= (transmittance / trans_pdf);

        if (!scatter && vertex_) {
            PathVertex vertex = *vertex_;
            if (never_scatter) {
                if (is_light(scene.shapes[vertex.shape_id])) {
                    radiance += current_path_throughput * emission(vertex, -ray.dir, scene);
                }
            } else {
                if (is_light(scene.shapes[vertex.shape_id])) {
                    PointAndNormal light_point = {vertex.position, vertex.geometric_normal};
                    int light_id = get_area_light_id(scene.shapes[vertex.shape_id]);
                    Light currlight = scene.lights[light_id];
                    Real pdf_nee = pdf_point_on_light(currlight, light_point, nee_p_cache, scene) * light_pmf(scene, light_id);
                    Vector3 omega_prime = normalize(vertex.position - nee_p_cache);
                    Real top = abs(dot(omega_prime, vertex.geometric_normal));
                    Real bottom = length_squared(vertex.position - nee_p_cache);
                    
                    Real G = top/bottom;
                    Real dir_pdf_ = dir_pdf * multi_trans_pdf * G;
                    
                    Real w = (dir_pdf_ * dir_pdf_) / (dir_pdf_ * dir_pdf_ + pdf_nee * pdf_nee);
                    
                    radiance += current_path_throughput * emission(vertex, -ray.dir, scene) * w;
                }
            }
        }

        if (bounces == max_depth - 1 && max_depth != -1) {
            break;
        }

        if (!scatter && vertex_) {
            PathVertex vertex = *vertex_;
            // if index matching surface
            if (vertex.material_id == -1) {
                current_medium = update_medium(vertex, ray);
                bounces += 1;
                ray.org = vertex.position + ray.dir * get_intersection_epsilon(scene);
                multi_trans_pdf *= trans_pdf;
                continue;
            }
        }

        if (scatter) {
            never_scatter = false;
            Vector3 p = ray.org;
            PhaseFunction pf = get_phase_function(scene.media[current_medium]);
            std::optional<Vector3> next_dir_ = sample_phase_function(pf, -ray.dir, Vector2(next_pcg32_real<Real>(rng), next_pcg32_real<Real>(rng)));
            if (next_dir_) {
                Vector3 next_dir = *next_dir_;
                Real sigma_s = get_sigma_s(scene.media[current_medium], p).x;

                int light_id = sample_light(scene, next_pcg32_real<Real>(rng));
                Spectrum nee_out = nee(p, -ray.dir, current_medium, bounces, scene.lights[light_id], scene, rng, light_id);

                radiance += current_path_throughput * nee_out * sigma_s;

                dir_pdf = pdf_sample_phase(pf, -ray.dir, next_dir);
                current_path_throughput *= (eval(pf, -ray.dir, next_dir) / dir_pdf) * sigma_s;

                ray = Ray{ ray.org + next_dir * get_intersection_epsilon(scene), next_dir, Real(0), infinity<Real>() };
                nee_p_cache = p;
                multi_trans_pdf = Real(1);
            }
        }
        else {
            //handle brdf

            if(vertex_) {

                PathVertex vertex = *vertex_;

                int light_id = sample_light(scene, next_pcg32_real<Real>(rng));
                Spectrum nee_out = nee_brdf(ray.org, -ray.dir, current_medium, bounces, scene.lights[light_id], scene, rng, light_id, vertex);

                radiance += current_path_throughput * nee_out;
                never_scatter = false;
                
                const Material &mat = scene.materials[vertex.material_id];
                Vector3 dir_view = -ray.dir;
                Vector2 bsdf_rnd_param_uv{next_pcg32_real<Real>(rng), next_pcg32_real<Real>(rng)};
                Real bsdf_rnd_param_w = next_pcg32_real<Real>(rng);
                std::optional<BSDFSampleRecord> bsdf_sample_ =
                    sample_bsdf(mat,
                                dir_view,
                                vertex,
                                scene.texture_pool,
                                bsdf_rnd_param_uv,
                                bsdf_rnd_param_w);
                if (!bsdf_sample_) {
                    // BSDF sampling failed. Abort the loop.
                    break;
                }

                const BSDFSampleRecord &bsdf_sample = *bsdf_sample_;
                Vector3 dir_bsdf = bsdf_sample.dir_out;

                ray = Ray{ray.org, dir_bsdf, get_intersection_epsilon(scene), infinity<Real>()};
                
                current_medium = update_medium(vertex, ray);

                Spectrum bsdf = eval(mat, dir_view, dir_bsdf, vertex, scene.texture_pool);
                Real bsdf_pdf = pdf_sample_bsdf(mat, dir_view, dir_bsdf, vertex, scene.texture_pool);
                current_path_throughput *= bsdf / bsdf_pdf;


                //update cache
                dir_pdf = bsdf_pdf;
                nee_p_cache = ray.org;
                multi_trans_pdf = Real(1);

            }
        }

        Real rr_prob = 1;

        if (bounces >= max_depth) {
            rr_prob = min(current_path_throughput.x, 0.95);
            if (next_pcg32_real<Real>(rng) > rr_prob) {
                break;
            }
            else {
                current_path_throughput /= rr_prob;
            }
        }
        bounces++;
    }

    return radiance;
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
