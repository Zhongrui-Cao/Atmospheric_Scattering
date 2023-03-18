#pragma once

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
    Real multi_trans_pdf = 1;
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
        Real trans_pdf = 1;
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

            // Assume monochromatic medium
            Real t = -log(1 - next_pcg32_real<Real>(rng)) / sigma_t[2];
            if (t < max_t) {
                scatter = true; // Tell the code to sample the phase function later
                never_scatter = false;

                // Update vertex information
                vertex.position = ray.org + t * ray.dir;
                vertex.interior_medium_id = current_medium_id;
                vertex.exterior_medium_id = current_medium_id;

                transmittance = exp(-sigma_t * t);
                trans_pdf = exp(-sigma_t[2] * t) * sigma_t[2];
            } else { // t >= max_t
                if (!surface_vertex) {
                    // do we need this?
                    vertex.position = ray.org + max_t * ray.dir;
                    vertex.interior_medium_id = current_medium_id;
                    vertex.exterior_medium_id = current_medium_id;
                }
                transmittance = exp(-sigma_t * max_t);
                trans_pdf = exp(-sigma_t[2] * max_t);
            }
        }
        // Update multiple bounces transmittion pdf.
        multi_trans_pdf *= trans_pdf;

        // Update current path throughput with transmittance
        current_path_throughput *= (transmittance / trans_pdf);

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
                        Real p_dir = dir_pdf * multi_trans_pdf * G;
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
        multi_trans_pdf = 1;

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
            Real p_trans_dir = 1;
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
                    p_trans_dir *= exp(-sigma_t[2] * next_t);
                }

                if (!shadow_vertex) {
                    // Nothing is blocking, we're done
                    break;
                } else {
                    // Something is blocking: is it an opaque surface?
                    if (shadow_vertex->material_id >= 0) {
                        // we're blocked
                        T_light = make_zero_spectrum();
                        p_trans_dir = 0;
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
                p2 *= p_trans_dir;
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
                                      phase_rnd_param_uv);
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

// The final volumetric renderer: 
// multiple chromatic heterogeneous volumes with multiple scattering
// with MIS between next event estimation and phase function sampling
// with surface lighting
Spectrum vol_path_rayleigh(const Scene& scene,
    int x, int y, /* pixel coordinates */
    pcg32_state& rng) {
    // Homework 2: implememt this!
    int w = scene.camera.width, h = scene.camera.height;
    Vector2 screen_pos((x + next_pcg32_real<Real>(rng)) / w,
        (y + next_pcg32_real<Real>(rng)) / h);
    Ray ray = sample_primary(scene.camera, screen_pos);
    RayDifferential ray_diff = RayDifferential{ Real(0), Real(0) };
    int current_medium_id = scene.camera.medium_id;
    Spectrum radiance = make_zero_spectrum();
    int max_depth = scene.options.max_depth;
    Spectrum current_path_throughput = fromRGB(Vector3{ 1, 1, 1 });
    Real dir_pdf = 0; // in solid angle measure
    Vector3 nee_p_cache;
    Spectrum multi_trans_dir_pdf = make_const_spectrum(1);
    Spectrum multi_trans_nee_pdf = make_const_spectrum(1);
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
        // Now we have different transmittance sampling strategy for each channel,
        // so the PDF is a Spectrum instead of a Real.
        Spectrum trans_dir_pdf = make_const_spectrum(1);
        Spectrum trans_nee_pdf = make_const_spectrum(1);
        if (current_medium_id >= 0) {
            const Medium& medium = scene.media[current_medium_id];
            // In order to sample a distance, we adopt the null-scattering sampling strategy
            // from Galtier et al. "Radiative transfer and spectroscopic databases: A line-sampling Monte Carlo approach"
            // The idea is like this: we have the following radiative transfer equation:
            // L' = -sigma_t L + sigma_a L_e + sigma_s \int_{S^2} p L dw
            // integrating over distance we have the usual volume rendering equation:
            // L = \int_{0}^{z} T [sigma_a L_e + sigma_s \int_{S^2} p L dw] dz'
            // where T = exp(-\int_{0}^{z'} sigma_t dz'')
            // The exponentiation of T makes it difficult to estimate this integral
            // using Monte Carlo when sigma_t is varying with z''.
            // To deal with this, we use a trick called "homogenization":
            // we modify the RTE as follows:
            // L' = -(sigma_t + sigma_n)L + (sigma_a L_e + sigma_s \int_{S^2} p L dw + sigma_n L)
            // Note that this is just subtracting sigma_n first and then adding it
            // However, if we set sigma_n = M - sigma_t where M is some constant 
            // which is an upperbound of sigma_t (usually called "majorant"),
            // the rendering equation becomes:
            // L = \int_{0}^{z} T_m [sigma_a L_e + sigma_s \int_{S^2} p L dw + sigma_n L] dz' + T_m[0->z] sigma_t L_s
            // where T_m = exp(-\int_{0}^{z'} M dz'') has a simple close-form solution.
            // An alternative way to look at this is to look at an alternative volume rendering equation,
            // by simply integrating the both sides of the RTE:
            // L = \int -sigma_t L + sigma_a L_e + sigma_s \int_{S^2} p L dw
            // then we apply control variates to this integral to obtain the null-scattering RTE.

            // Given L = \int_{0}^{z} T_m [sigma_a L_e + sigma_s \int_{S^2} p L dw + sigma_n L] dz'
            // We can then importance sample T_m, and then randomly chooses which term we want to 
            // estimate in the [] bracket (we need to do this, otherwise we will have exponential
            // path branching since there are 2 recursive Ls). 

            // We choose between the three terms based on sigma_a, sigma_s and sigma_n.
            // Furthermore, since our volumes are not emissive, we know that L_e = 0,
            // so we only need to sample the sigma_s term and sigma_n term.
            // Therefore we combine the scatter event and absorption event into a "collision event"
            // the collision event happens at probability sigma_t / majorant

            // Now we face a problem: the sigma_a/sigma_s/majorant have different values
            // at different wavelengthes! The above derivation only applies when
            // they are scalars.
            // Therefore in addition to sampling a distance, we also need to sample
            // a color component.
            // We also want to perform multiple importance sampling among all channels.
            // Thus we need to keep track of all PDFs in our stochastic evaluation.
            // This is discussed in
            // "A null-scattering path integral formulation of light transport"
            // from Miller et al.            

            Real max_t = infinity<Real>();
            if (surface_vertex) {
                max_t = distance(surface_vertex->position, ray.org);
            }
            Spectrum majorant = get_majorant(medium, ray);

            // Sample a channel for sampling
            Real u = next_pcg32_real<Real>(rng);
            int channel = std::clamp(int(u * Real(3)), 0, 2);
            Real ma_c = majorant[channel];
            Real accum_t = 0;
            for (int count = 0; count < scene.options.max_null_collisions && ma_c > 0; count++) {
                Real t = -log(1 - next_pcg32_real<Real>(rng)) / ma_c;
                Real dt = max_t - accum_t;
                // Update accumulated distance
                accum_t = min(accum_t + t, max_t);
                if (t < dt) { // Haven't reached a surface yet
                    Vector3 p = ray.org + accum_t * ray.dir;
                    Spectrum sigma_a = get_sigma_a(medium, p);
                    Spectrum sigma_s = get_sigma_s(medium, p);
                    Spectrum sigma_t = sigma_s + sigma_a;
                    Spectrum sigma_n = majorant - sigma_t;
                    // With probability sigma_t / majorant, we sample a 
                    // collision event and end the loop. Otherwise
                    // we continue sampling since we hit a null particle
                    // For numerical stability, we divide everything by the maximum of the majorant

                    Real collision_prob = sigma_t[channel] / ma_c;
                    if (next_pcg32_real<Real>(rng) <= collision_prob) {
                        scatter = true; // Tell the code to sample the phase function later
                        never_scatter = false;

                        // sigma_t will be accounted for later
                        transmittance *= exp(-majorant * t) / max(majorant);
                        trans_dir_pdf *= exp(-majorant * t) * sigma_t / max(majorant);
                        // Update vertex information
                        vertex.position = ray.org + accum_t * ray.dir;
                        vertex.interior_medium_id = current_medium_id;
                        vertex.exterior_medium_id = current_medium_id;
                        break;
                    }
                    else {
                        transmittance *= exp(-majorant * t) * sigma_n / max(majorant);
                        trans_dir_pdf *= exp(-majorant * t) * sigma_n / max(majorant);
                        trans_nee_pdf *= exp(-majorant * t) * majorant / max(majorant);
                    }
                }
                else { // t >= max_t
                 // Only a single event when we reach the end
                    transmittance *= exp(-majorant * dt);
                    trans_dir_pdf *= exp(-majorant * dt);
                    trans_nee_pdf *= exp(-majorant * dt);
                    // No need to update vertex information: it's already correct
                    break;
                }
            }
        }
        // Update multiple bounces transmittion pdf.
        multi_trans_dir_pdf *= trans_dir_pdf;
        multi_trans_nee_pdf *= trans_nee_pdf;

        // Update current path throughput with transmittance
        current_path_throughput *= (transmittance / average(trans_dir_pdf));

        // If we didn't scatter and didn't reach a surface either,
        // we're done.
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
                    }
                    else {
                        // Need to account for next event estimation
                        int light_id = get_area_light_id(scene.shapes[vertex.shape_id]);
                        assert(light_id >= 0);
                        const Light& light = scene.lights[light_id];
                        PointAndNormal light_point{ vertex.position, vertex.geometric_normal };
                        // Note that p_nee needs to account for the path vertex that issued
                        // next event estimation potentially many bounces ago.
                        // The vertex position is stored in nee_p_cache.
                        // We also need to account for the ratio tracking pdf,
                        // which waas cached in multi_trans_nee_pdf
                        Real p_nee = light_pmf(scene, light_id) *
                            pdf_point_on_light(light, light_point, nee_p_cache, scene) *
                            average(multi_trans_nee_pdf);
                        // The PDF for sampling the light source using directional sampling + transmittance sampling
                        // The directional sampling pdf was cached in dir_pdf in solid angle measure.
                        // The transmittance sampling pdf was cached in multi_trans_dir_pdf.
                        Vector3 light_dir = normalize(vertex.position - nee_p_cache);
                        Real G = fabs(dot(vertex.geometric_normal, light_dir)) /
                            distance_squared(nee_p_cache, vertex.position);
                        Real p_dir = dir_pdf * average(multi_trans_dir_pdf) * G;
                        Real w2 = (p_dir * p_dir) / (p_dir * p_dir + p_nee * p_nee);
                        // current_path_throughput already accounts for dir_pdf & transmittance.
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
                ray = Ray{ vertex.position,
                          ray.dir,
                          get_intersection_epsilon(scene),
                          infinity<Real>() };
                current_medium_id =
                    update_medium_id(vertex, ray, current_medium_id);
                continue;
            }
        }

        // Cache the NEE vertex for later use
        nee_p_cache = vertex.position;
        // We have a scattering event (medium or surface), reset multi_trans_pdfs
        multi_trans_dir_pdf = make_const_spectrum(1);
        multi_trans_nee_pdf = make_const_spectrum(1);

        // next event estimation
        Spectrum C1 = make_zero_spectrum();
        Real w1 = 0;
        {
            // Sample a point on the light source.
            Vector2 light_uv{ next_pcg32_real<Real>(rng), next_pcg32_real<Real>(rng) };
            Real light_w = next_pcg32_real<Real>(rng);
            Real shape_w = next_pcg32_real<Real>(rng);
            int light_id = sample_light(scene, light_w);
            const Light& light = scene.lights[light_id];
            PointAndNormal point_on_light =
                sample_point_on_light(light, vertex.position, light_uv, shape_w, scene);
            // Compute transmittance to light. Skip through index-matching shapes.
            Spectrum T_light = make_const_spectrum(1);
            Vector3 p = vertex.position;
            int shadow_medium_id = current_medium_id;
            int shadow_bounces = 0;
            Spectrum p_trans_nee = make_const_spectrum(1);
            Spectrum p_trans_dir = make_const_spectrum(1);
            while (true) {
                Vector3 dir_light = normalize(point_on_light.position - p);
                Ray shadow_ray{ p, dir_light,
                               get_shadow_epsilon(scene),
                               (1 - get_shadow_epsilon(scene)) *
                                   distance(point_on_light.position, p) };
                std::optional<PathVertex> shadow_vertex = intersect(scene, shadow_ray);
                Real next_t = shadow_ray.tfar;
                if (shadow_vertex) {
                    next_t = distance(p, shadow_vertex->position);
                }

                // Account for the transmittance to next_t
                // Unlike previous implementations, here we use "ratio tracking"
                // for estimating the transmittance. The idea is to 
                // estimate exp(-\int_{0}^{max_t} sigma_t)
                // by sampling a distance t according to the majorant,
                // then weigh the samples using (majorant - sigma_t) / majorant
                // if t < max_t, otherwise weight them with sigma_t / majorant.
                // For numerical stability, we divide everything by the maximum of the majorant
                if (shadow_medium_id >= 0) {
                    const Medium& medium = scene.media[shadow_medium_id];
                    Spectrum majorant = get_majorant(medium, shadow_ray);

                    // Sample a channel for sampling
                    Real u = next_pcg32_real<Real>(rng);
                    int channel = std::clamp(int(u * Real(3)), 0, 2);
                    Real ma_c = majorant[channel];
                    Real accum_t = 0;
                    for (int count = 0; count < scene.options.max_null_collisions && ma_c > 0; count++) {
                        Real t = -log(1 - next_pcg32_real<Real>(rng)) / ma_c;
                        Real dt = next_t - accum_t;
                        accum_t = min(accum_t + t, next_t);
                        Vector3 p = shadow_ray.org + accum_t * shadow_ray.dir;
                        Spectrum sigma_a = get_sigma_a(medium, p);
                        Spectrum sigma_s = get_sigma_s(medium, p);
                        Spectrum sigma_t = sigma_s + sigma_a;
                        Spectrum sigma_n = majorant - sigma_t;
                        if (t < dt) {
                            // didn't hit the surface, so this is a null-scattering event
                            T_light *= exp(-majorant * t) * sigma_n / max(majorant);
                            p_trans_nee *= exp(-majorant * t) * majorant / max(majorant);
                            p_trans_dir *= exp(-majorant * t) * sigma_n / max(majorant);
                            if (max(T_light) <= 0) {
                                break;
                            }
                        }
                        else { // t >= next_t: we hit a surface
                         // account for the probability of sampling more than next_t
                            T_light *= exp(-majorant * dt);
                            p_trans_nee *= exp(-majorant * dt);
                            p_trans_dir *= exp(-majorant * dt);
                            break;
                        }
                    }
                }

                if (!shadow_vertex) {
                    // Nothing is blocking, we're done
                    break;
                }
                else {
                    // Something is blocking: is it an opaque surface?
                    if (shadow_vertex->material_id >= 0) {
                        // we're blocked
                        T_light = make_zero_spectrum();
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
                    shadow_medium_id = update_medium_id(*shadow_vertex, shadow_ray, shadow_medium_id);
                    p = p + next_t * dir_light;
                }
            }

            if (max(T_light) > 0 && max(p_trans_nee) > 0) {
                // Compute sigma_s * T * T_light * G * f * L
                Vector3 dir_light = normalize(point_on_light.position - vertex.position);
                Real G = max(-dot(dir_light, point_on_light.normal), Real(0)) /
                    distance_squared(point_on_light.position, vertex.position);
                Real p1 = light_pmf(scene, light_id) *
                    pdf_point_on_light(light, point_on_light, vertex.position, scene) *
                    average(p_trans_nee);
                Vector3 dir_view = -ray.dir;
                Spectrum f;
                // are we on a surface or are we in a medium?
                if (scatter) {
                    assert(current_medium_id >= 0);
                    const Medium& medium = scene.media[current_medium_id];
                    const PhaseFunction& phase_function = get_phase_function(medium);
                    f = eval(phase_function, dir_view, dir_light);
                }
                else {
                    const Material& mat = scene.materials[vertex.material_id];
                    f = eval(mat, dir_view, dir_light, vertex, scene.texture_pool);
                }
                Spectrum sigma_s = make_const_spectrum(1);
                if (scatter) {
                    const Medium& medium = scene.media[current_medium_id];
                    sigma_s = get_sigma_s(medium, vertex.position);
                }
                Spectrum L = emission(light, -dir_light, Real(0), point_on_light, scene);
                C1 = current_path_throughput * sigma_s * T_light * G * f * L / p1;
                // Multiple importance sampling: it's also possible
                // that a phase function sampling + multiple steps exponential sampling
                // will reach the light source.
                Real p2 = 0;
                if (scatter) {
                    assert(current_medium_id >= 0);
                    const Medium& medium = scene.media[current_medium_id];
                    const PhaseFunction& phase_function = get_phase_function(medium);
                    p2 = pdf_sample_phase(phase_function, dir_view, dir_light) * G;
                }
                else {
                    assert(vertex.material_id >= 0);
                    const Material& mat = scene.materials[vertex.material_id];
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
            const Medium& medium = scene.media[current_medium_id];
            const PhaseFunction& phase_function = get_phase_function(medium);
            Vector3 dir_view = -ray.dir;
            Vector2 phase_rnd_param_uv{ next_pcg32_real<Real>(rng), next_pcg32_real<Real>(rng) };
            std::optional<Vector3> phase_sample =
                sample_phase_function(phase_function,
                    dir_view,
                    phase_rnd_param_uv);
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
        }
        else {
            // BSDF sampling
            const Material& mat = scene.materials[vertex.material_id];
            Vector3 dir_view = -ray.dir;
            Vector2 bsdf_rnd_param_uv{ next_pcg32_real<Real>(rng), next_pcg32_real<Real>(rng) };
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
        ray = Ray{ vertex.position,
                  next_dir,
                  get_intersection_epsilon(scene),
                  infinity<Real>() };
        current_medium_id =
            update_medium_id(vertex, ray, current_medium_id);
    }
    return radiance;
}

