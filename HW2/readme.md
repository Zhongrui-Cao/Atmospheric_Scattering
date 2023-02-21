## 2 Single monochromatic absorption-only homogeneous volume

1. Change the absorption parameters to zero in scenes/volpath_test/volpath_test1.xml. What do you see? Why?  

    I see the emision of the sphere get brighter. This is because when the absorption gets smaller, the 
L1 term will also get bigger so less light get absorped therefore brghter.  

2. In the homework, we assume the volume being not emissive. If you were tasked to modify the pseudo
code above to add volume emission, how would you do it? Briefly describe your approach.  

    We can integrate Le over dt to get emission since this is homogeneous medium. We can use similar
stradegy as before, first sample t, and add emission as e^-sigma_a*t * Le.

## 3 Single monochromatic homogeneous volume with absorption and single-scattering, no surface lighting

1. In the derivation above, how did we get from p(t) ∝ exp(−σtt) to p(t) = σt exp(σtt)?

    if we integrate pdf:  
$ \int_0^\infty \exp(-\sigma_t s)ds = 0 + \frac{1}{\sigma_t}$  
To make this integrate to 1 we need to make $pdf = \sigma_t*exp(-\sigma_t*t)$

2. How was Equation (13) incorporated into the pseudo code above? Why is it done this way?  

    This is used to calculate trans_pdf. We will divede the tranmittance by this pdf. However the transmittance is the same as trans_pdf, so effectively we just return Le in the end.

3. Play with the parameters σs and σa, how do they affect the final image? Why? (we assume monochro-
matic volumes, so don’t set the parameters to be different for each channel.)  

    For both parameters, if we set it higher, there will be more lights loss. For sigma_a, if we set it higher, the image will get significantly darker, this is because sigma_a is the absorption term, more means more light get absorbed. For sigma_s, it is the scattering term. If we set it higher, the medium will get more scattery, therefore a little darker, but more blured and diffuse looking.

4. Change the phase function from isotropic (the default one) to Henyey-Greenstein by uncommenting
the phase function specification in the scene scenes/volpath_test/volpath_test2.xml. Play with the
parameter g (the valid range is (−1, 1)). What does g mean? How does the g parameter change the
appearance? Why?  

    If g is positive, this means the medium will scatter more light in the forward direction, if g is negative, the medium will scatter more light backwards. However if the value is higher, there will be less side light get scattered, therefore having a more transluscent but less scattering look. If g is negative the image will be less bright since our light direction is towards the camera.

## 4 Multiple monochromatic homogeneous volumes with absorption and multiple-scattering using only phase function sampling, no surface lighting

1. Play with the parameters σs, σa of different volumes, and change max_depth, how do they affect the
final image? How does increasing/decreasing σs and σa of medium1 and medium2 affect the appearance,
respectively? Why? Do different σs and σa values affect how high you should set max_depth?  

    If we increase sigma s, for the medium in the air, the brightness of the image will look more even and more blurry, for the sphere medium, it will look less transparent, since the light coming through will be scattered more. If we increase the sigma a, then we will get a generally darker medium, this is true for both the medium in air and the sphere. For max_depth, of course we will get darker image because energy loss, but it will affect higher sigma_s mediums more. This is because higher sigma_s mediums will require more scattering in the medium to have the correct apprance. For sigma_a this does not affect as much through my observation.

2. Switch to the Henyey-Greenstein phase function again. How does changing the g parameter affect the
appearance? Why?

    If the g term is negative, we will see less of a transparency through the sphere medium, this is because less light get through from back to front. However if the g term is positive, we will see brighter and more transparent medium because the lope is more concentrated to the front direction and scatter less.

3. Propose a phase function yourself (don’t have to describe the exact mathematical form). How would
you design the shape of the phase function? What parameter would you set to control it?  

    I would do something similar to the Henyey-Greenstein phase function, but I think I will do a six parameter control for fun. Each parameter controls a direction where the phase function is most strong, this way we have fine-grained control over the phase function.

## 5 Multiple monochromatic homogeneous volumes with absorption and multiple-scattering with both phase function sampling and next event estimation, no surface lighting

1. When will next event estimation be more efficient than phase function sampling? In our test scenes,
which one is more efficient? Why?

    Nee will be more efficient when the light source is small and far, since normal sampling will be harder to hit the light source. Our test scenes have this case, so it is more efficient to use nee.

2. In scenes/volpath_test/volpath_test4_2.xml, we render a scene with an object composed of dense
volume. How does it compare to rendering the object directly with a Lambertian material? Why are
they alike or different?  

    They pretty alike, since they have similar behavior when it comes to scattering incoming light in a random fashion. However, they are different because volumetric have the subsurface scattering built-in, but lambertian don't have this.

3. im Kajiya famously has predicted in 1991 that in 10 years, all rendering will be volume rendering.
What do you think that makes him think so? Why hasn’t it happened yet?  

    I think he said this because volume rendering is a really good phisical way of modeling complex rendering effects, and a lot of real world effect comes from volums, such as fog and skin and food and smoke and such. However our computation hardware is yet to be good enough to handle this efficiently, also it is harder for artistic control and it is harder to implement. If we have good ways of approximating this efficiently then there is no need of using volume.

## 6 Multiple monochromatic homogeneous volumes with absorption and multiple-scattering with both phase function sampling and next event estimation, with surface lighting

1. Play with the index of refraction parameter of the dielectric interface in
scenes/volpath_test/volpath_test5_2.xml. How does that affect appearance? Why?

    with low index of refaction, we cannot see the medium inside, the whole sphere look like a solid ball. I think this is because the light will pass through without being refracted. If we have the IOR higher we will see the medium inside more clearly because more light is refracted.

2. In the scene scenes/volpath_test/vol_cbox_teapot.xml, we model the glass teapot as a transparent
glass with blue homogeneous medium inside. What is the difference in terms of appearance between
this approach and just making the color of the glass blue without any medium inside?

    By modeling glass as a volume, we can get more complex effects such as color of glass resulting from scattering and absorbsion, which will vary based on the thickness of the glass. If we just make the color blue, we will not have this phisically correct effect.

## 7 Multiple chromatic heterogeneous volumes with absorption and multiple-scattering with both phase function sampling and next event estimation, with surface lighting

1. For heterogeneous volumes, what kind of distribution of the volume density makes the null scattering
efficient/inefficient? Can you think of a way to improve our current sampling scheme in the inefficient
case?  

    If the majorant we have is very big for the medium, then sampling will become inefficient. This happens when sigma_t have a lot of variation. We can solve this by sampling regions with similar sigma_t together and in the end combine everything together.

2. How do we make the null-scattering work for emissive volumes? Briefly describe a solution.

    We can do the same trick as before, where we homogenize the emissive volume and use null particles to do the emission as well. And in the end, we need to do some math magic to remove the fake emmisive contribution.

3. Why is it important to have an unbiased solution for volume rendering? Would it be sensible to have
something that is biased but faster? How would you do it?  

    I think for the sake of research and physically correct rendering, it is important to have unbiased solution. For example I imagine differentiable rendering for volume will require unbiasness. However if we are just visuallizing a simulation or rendering for art, a fast but biased approximation will make sense, because we don't care about the correctness but the time and cost and artistic result.