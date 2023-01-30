1. Diffuse
	1. Compare the two BRDFs with a Lambertian BRDF (try to render images with all three BRDFs):
    what differences do you see? Why?

    First, the Lambertian BRDF is brighter than both the diffuse component and the subsurface
    component of the Disney BRDF. Since the Disney BRDF is designed to be a linear combination
    of the two component, it makes sense that the singe components are darker.
    Comparing the single BRDFs, I observed no difference between the lambertian and the subsurface
    component visually other than the fact that diffuse is darker. As for the diffuse component,
    I observed that at grazing angle it is darker than subsurface and lambert.
    Generally, the final combined BRDF is brighter than then Lambertian because it considers the 
    effect of subsurface scattering. The color is also more vibrant than Lambertian.

    ????????TODO?????????

	
	2. Play with the roughness parameter: how does it affect the appearance?

    With the roughness get higher (number smaller), the appearance will get darker.
    With the roughness get lower (number bigger), the appearance will get brighter.
    This is because of the energy loss when we don't consider multiple scattering. When the object is rough,
    it absorbs more light.
	
	3. Compare the base diffuse BRDF (fbaseDiffuse) with the subsurface BRDF (fsubsurface) by playing with
    the subsurface parameter. What differences do you see? Why? In what lighting condition does
    the base diffuse BRDF differ the most from the subsurface BRDF? (Play with the light position in
    simple_sphere.xml for your experimentation)



    4. (Optional, bonus 3%) Another popular option for modeling diffuse surfaces is the Oren-Nayar BRDF [9],
    which is used in the Autodesk’s Standard Surface BSDF. What is the difference between the Oren-
    Nayar BRDF and the Disney diffuse BRDF? What are the pros and cons? What is your preferece

2. Metal
    