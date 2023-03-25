#pragma once

const Real r_weight = 0.11060479323; //for 680nm 
const Real g_weight = 0.25843946974; //for 550nm
const Real b_weight = 0.63095573702; //for 440nm

inline Real weighted_average(const Vector3 &v) {
    return (v.x * r_weight + v.y * g_weight + v.z * b_weight);
}

inline int update_medium_id_rayleigh(const PathVertex &vertex,
                            const Ray &ray,
                            int current_medium_id) {
    if (vertex.interior_medium_id != vertex.exterior_medium_id) {
        // At medium transition. Update medium id.
        if (dot(ray.dir, vertex.geometric_normal) > 0) {
            current_medium_id = vertex.exterior_medium_id;
        } else {
            current_medium_id = vertex.interior_medium_id;
        }
    }
    return current_medium_id;
}

// The fifth volumetric renderer: 
// multiple monochromatic homogeneous volumes with multiple scattering
// with MIS between next event estimation and phase function sampling
// with surface lighting
Spectrum vol_path_rayleigh_5(const Scene &scene,
                            int x, int y, /* pixel coordinates */
                            pcg32_state &rng) {
    // Homework 2: implememt this!
    int w = scene.camera.width, h = scene.camera.height;
    Vector2 screen_pos((x + next_pcg32_real<Real>(rng)) / w,
                       (y + next_pcg32_real<Real>(rng)) / h);
    Ray ray = sample_primary(scene.camera, screen_pos);
    RayDifferential ray_diff = RayDifferential{Real(0), Real(0)};
    int current_medium_id = scene.camera.medium_id;
    Spectrum radiance = make_zero_spectrum();
    int max_depth = scene.options.max_depth;
    Spectrum current_path_throughput = fromRGB(Vector3{1, 1, 1});
    // To add contribution from phase function sampling (or directional sampling)
    // and combine it with next event estimation using MIS,
    // we need to keep track of the PDFs for both of them.
    // For directional sampling, we need to account for the phase function/BSDF sampling PDF.
    // For next event estimation, we need to account for the path vertex that 
    // issued the light source sampling and went through multiple index-matching interfaces.
    // For both sampling strategies, we need to account for the probability of
    // the transmittance sampling going through multiple index-matching interfaces.
    // All quantities needs to be accessed across multiple bounces,
    // For the direction sampling PDF, we store it in dir_pdf.
    // For the next event estimation path vertex, we store it in nee_p_cache.
    // For the transmittance sampling probability, we store it in multi_trans_pdf.
    Real dir_pdf = 0; // in solid angle measure
    Vector3 nee_p_cache;
    Spectrum multi_trans_pdf = make_const_spectrum(1);
    bool never_scatter = true;
    for (int bounces = 0; max_depth == -1 || bounces < max_depth; bounces++) {
        std::optional<PathVertex> surface_vertex = intersect(scene, ray, ray_diff);
        bool scatter = false;
        PathVertex vertex;
        if (surface_vertex) {
            vertex = *surface_vertex;
        }

        // Consider medium transmittance by sampling a distance along the medium.
        // Update "vertex". Compute a transmittance and its PDF.
        Spectrum transmittance = make_const_spectrum(1);
        Spectrum trans_pdf = make_const_spectrum(1);
        if (current_medium_id >= 0) {
            const Medium &medium = scene.media[current_medium_id];
            // We have an integral \int_{0}^{t} T(t') sigma_s f G L dt' + T(max_t) Le
            // T(t) = exp(-\int_{0}^{t} sigma_t dt'')
            // We'll importance sample T
            Real max_t = infinity<Real>();
            if (surface_vertex) {
                max_t = distance(surface_vertex->position, ray.org);
            }
            Spectrum sigma_a = get_sigma_a(medium, ray.org);
            Spectrum sigma_s = get_sigma_s(medium, ray.org);
            Spectrum sigma_t = sigma_s + sigma_a;

            // Importance Sample a channel for sampling
            Real u = next_pcg32_real<Real>(rng);
            int channel = std::clamp(int(u * Real(3)), 0, 2);
            
            /*
            int channel = 0;
            if (u < r_weight) {
                channel = 0;
            } else if (u < r_weight + b_weight) {
                channel = 1;
            } else {
                channel = 2;
            }
            */
            
            // do not Assume monochromatic medium
            Real t = -log(1 - next_pcg32_real<Real>(rng)) / sigma_t[channel];
            if (t < max_t) {
                scatter = true; // Tell the code to sample the phase function later
                never_scatter = false;

                // Update vertex information
                vertex.position = ray.org + t * ray.dir;
                vertex.interior_medium_id = current_medium_id;
                vertex.exterior_medium_id = current_medium_id;

                transmittance = exp(-sigma_t * t);
                trans_pdf = exp(-sigma_t * t) * sigma_t;
            } else { // t >= max_t
                if (!surface_vertex) {
                    // do we need this?
                    vertex.position = ray.org + max_t * ray.dir;
                    vertex.interior_medium_id = current_medium_id;
                    vertex.exterior_medium_id = current_medium_id;
                }
                transmittance = exp(-sigma_t * max_t);
                trans_pdf = exp(-sigma_t * max_t);
            }
        }
        // Update multiple bounces transmittion pdf.
        multi_trans_pdf *= trans_pdf;

        // Update current path throughput with transmittance
        current_path_throughput *= (transmittance / average(trans_pdf));

        // If we didn't scatter and didn't reach a surface either,
        // we're done
        if (!scatter && !surface_vertex) {
            break;
        }

        // If we reach a surface and didn't scatter, include the emission.
        if (!scatter) {
            if (surface_vertex) {
                if (is_light(scene.shapes[vertex.shape_id])) {
                    if (never_scatter) {
                        // This is the only way we can see the light source, so
                        // we don't need multiple importance sampling.
                        radiance += current_path_throughput *
                            emission(vertex, -ray.dir, scene);
                    } else {
                        // Need to account for next event estimation
                        int light_id = get_area_light_id(scene.shapes[vertex.shape_id]);
                        assert(light_id >= 0);
                        const Light &light = scene.lights[light_id];
                        PointAndNormal light_point{vertex.position, vertex.geometric_normal};
                        // Note that p_nee needs to account for the path vertex that issued
                        // next event estimation potentially many bounces ago.
                        // The vertex position is stored in nee_p_cache.
                        Real p_nee = light_pmf(scene, light_id) *
                            pdf_point_on_light(light, light_point, nee_p_cache, scene);
                        // The PDF for sampling the light source using directional sampling + transmittance sampling
                        // The directional sampling pdf was cached in dir_pdf in solid angle measure.
                        // The transmittance sampling pdf was cached in multi_trans_pdf.
                        Vector3 light_dir = normalize(vertex.position - nee_p_cache);
                        Real G = fabs(dot(vertex.geometric_normal, light_dir)) /
                            distance_squared(nee_p_cache, vertex.position);
                        Real p_dir = dir_pdf * average(multi_trans_pdf) * G;
                        Real w2 = (p_dir * p_dir) / (p_dir * p_dir + p_nee * p_nee);
                        // current_path_throughput already accounts for prev_dir_pdf & transmittance.
                        radiance += current_path_throughput *
                            emission(vertex, -ray.dir, scene) * w2;
                    }
                }
            }
        }

        if (max_depth != -1 && bounces == max_depth - 1) {
            break;
        }

        // If we reach a surface and it is index-matching, skip through it
        // if we hit nothing and we're not scattering, terminate.
        if (!scatter) {
            if (surface_vertex->material_id == -1) {
                ray = Ray{vertex.position,
                          ray.dir,
                          get_intersection_epsilon(scene),
                          infinity<Real>()};
                current_medium_id =
                    update_medium_id_rayleigh(vertex, ray, current_medium_id);
                continue;
            }
        }

        // Cache the NEE vertex for later use
        nee_p_cache = vertex.position;
        // We have a scattering event (medium or surface), reset multi_trans_pdf
        multi_trans_pdf = make_const_spectrum(1);

        // next event estimation
        Spectrum C1 = make_zero_spectrum();
        Real w1 = 0;
        {
            // Sample a point on the light source.
            Vector2 light_uv{next_pcg32_real<Real>(rng), next_pcg32_real<Real>(rng)};
            Real light_w = next_pcg32_real<Real>(rng);
            Real shape_w = next_pcg32_real<Real>(rng);
            int light_id = sample_light(scene, light_w);
            const Light &light = scene.lights[light_id];
            PointAndNormal point_on_light =
                sample_point_on_light(light, vertex.position, light_uv, shape_w, scene);
            // Compute transmittance to light. Skip through index-matching shapes.
            Spectrum T_light = make_const_spectrum(1);
            Vector3 p = vertex.position;
            int shadow_medium_id = current_medium_id;
            int shadow_bounces = 0;
            Spectrum p_trans_dir = make_const_spectrum(1);
            while (true) {
                Vector3 dir_light = normalize(point_on_light.position - p);
                Ray shadow_ray{p, dir_light, 
                               get_shadow_epsilon(scene),
                               (1 - get_shadow_epsilon(scene)) *
                                   distance(point_on_light.position, p)};
                std::optional<PathVertex> shadow_vertex = intersect(scene, shadow_ray);
                Real next_t = shadow_ray.tfar;
                if (shadow_vertex) {
                    next_t = distance(p, shadow_vertex->position);
                }

                // Account for the transmittance to next_t
                if (shadow_medium_id >= 0) {
                    const Medium &medium = scene.media[shadow_medium_id];
                    Spectrum sigma_a = get_sigma_a(medium, ray.org);
                    Spectrum sigma_s = get_sigma_s(medium, ray.org);
                    Spectrum sigma_t = sigma_s + sigma_a;

                    T_light *= exp(-sigma_t * next_t);
                    p_trans_dir *= exp(-sigma_t * next_t);
                }

                if (!shadow_vertex) {
                    // Nothing is blocking, we're done
                    break;
                } else {
                    // Something is blocking: is it an opaque surface?
                    if (shadow_vertex->material_id >= 0) {
                        // we're blocked
                        T_light = make_zero_spectrum();
                        p_trans_dir = make_zero_spectrum();
                        break;
                    }
                    // otherwise, we want to pass through -- this introduces
                    // one extra connection vertex
                    shadow_bounces++;
                    if (max_depth != -1 && bounces + shadow_bounces + 1 >= max_depth) {
                        // Reach the max no. of vertices
                        T_light = make_zero_spectrum();
                        break;
                    }
                    // let's update and continue
                    shadow_medium_id = update_medium_id_rayleigh(*shadow_vertex, shadow_ray, shadow_medium_id);
                    p = p + next_t * dir_light;
                }
            }
            
            if (max(T_light) > 0) {
                // Compute sigma_s * T * T_light * G * f * L
                Vector3 dir_light = normalize(point_on_light.position - vertex.position);
                Real G = max(-dot(dir_light, point_on_light.normal), Real(0)) /
                        distance_squared(point_on_light.position, vertex.position);
                Real p1 = light_pmf(scene, light_id) *
                    pdf_point_on_light(light, point_on_light, vertex.position, scene);
                Vector3 dir_view = -ray.dir;
                Spectrum f;
                // are we on a surface or are we in a medium?
                if (scatter) {
                    assert(current_medium_id >= 0);
                    const Medium &medium = scene.media[current_medium_id];
                    const PhaseFunction &phase_function = get_phase_function(medium);
                    f = eval(phase_function, dir_view, dir_light);
                } else {
                    const Material &mat = scene.materials[vertex.material_id];
                    f = eval(mat, dir_view, dir_light, vertex, scene.texture_pool);
                }
                Spectrum sigma_s = make_const_spectrum(1);
                if (scatter) {
                    const Medium &medium = scene.media[current_medium_id];
                    sigma_s = get_sigma_s(medium, vertex.position);
                }
                Spectrum L = emission(light, -dir_light, Real(0), point_on_light, scene);
                C1 = current_path_throughput * sigma_s * T_light * G * f * L / p1;
                // Multiple importance sampling: it's also possible
                // that a phase function sampling + multiple steps exponential sampling
                // will reach the light source.
                // The probability for multiple steps exponential sampling
                // is stored in p_trans_dir
                // We also need to multiply with G to convert phase function PDF to area measure.
                Real p2 = 0;
                if (scatter) {
                    assert(current_medium_id >= 0);
                    const Medium &medium = scene.media[current_medium_id];
                    const PhaseFunction &phase_function = get_phase_function(medium);
                    p2 = pdf_sample_phase(phase_function, dir_view, dir_light) * G;
                } else {
                    assert(vertex.material_id >= 0);
                    const Material &mat = scene.materials[vertex.material_id];
                    p2 = pdf_sample_bsdf(mat, dir_view, dir_light, vertex, scene.texture_pool) * G;
                }
                p2 *= average(p_trans_dir);
                w1 = (p1 * p1) / (p1 * p1 + p2 * p2);
            }
        }
        radiance += C1 * w1;

        // Sample the next direction & update current_path_throughput
        Vector3 next_dir;
        if (scatter) {
            // Phase function sampling
            assert(current_medium_id >= 0);
            const Medium &medium = scene.media[current_medium_id];
            const PhaseFunction &phase_function = get_phase_function(medium);
            Vector3 dir_view = -ray.dir;
            Vector2 phase_rnd_param_uv{next_pcg32_real<Real>(rng), next_pcg32_real<Real>(rng)};
            std::optional<Vector3> phase_sample =
                sample_phase_function(phase_function,
                                      dir_view,
                                      phase_rnd_param_uv, 
                                      next_pcg32_real<Real>(rng));
            if (!phase_sample) {
                // Phase function sampling failed. Abort the loop.
                break;
            }
            next_dir = *phase_sample;
            Spectrum f = eval(phase_function, dir_view, next_dir);
            Real p2 = pdf_sample_phase(phase_function, dir_view, next_dir);
            dir_pdf = p2;
            // Need to remember multiplying the scattering coefficient!
            Spectrum sigma_s = get_sigma_s(medium, vertex.position);
            current_path_throughput *= sigma_s * (f / p2);
        } else {
            // BSDF sampling
            const Material &mat = scene.materials[vertex.material_id];
            Vector3 dir_view = -ray.dir;
            Vector2 bsdf_rnd_param_uv{next_pcg32_real<Real>(rng), next_pcg32_real<Real>(rng)};
            Real bsdf_rnd_param_w = next_pcg32_real<Real>(rng);
            std::optional<BSDFSampleRecord> bsdf_sample =
                sample_bsdf(mat, dir_view, vertex, scene.texture_pool,
                    bsdf_rnd_param_uv, bsdf_rnd_param_w);
            if (!bsdf_sample) {
                // Phase function sampling failed. Abort the loop.
                break;
            }
            next_dir = bsdf_sample->dir_out;
            Spectrum f = eval(mat, dir_view, next_dir, vertex, scene.texture_pool);
            Real p2 = pdf_sample_bsdf(mat, dir_view, next_dir, vertex, scene.texture_pool);
            dir_pdf = p2;
            // Need to remember multiplying the scattering coefficient!
            current_path_throughput *= (f / p2);
            never_scatter = false;
        }

        // Update rays/current_path_throughput/current_pdf
        // Russian roulette heuristics
        Real rr_prob = 1;
        if (bounces >= scene.options.rr_depth) {
            rr_prob = min(max(current_path_throughput), Real(0.95));
            if (next_pcg32_real<Real>(rng) > rr_prob) {
                // Terminate the path
                break;
            }
            current_path_throughput /= rr_prob;
        }

        // Update rays
        ray = Ray{vertex.position,
                  next_dir,
                  get_intersection_epsilon(scene),
                  infinity<Real>()};
        current_medium_id =
            update_medium_id_rayleigh(vertex, ray, current_medium_id);
    }
    return radiance;
}
