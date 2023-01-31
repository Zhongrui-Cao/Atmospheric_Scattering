1. Diffuse
	1. Compare the two BRDFs with a Lambertian BRDF (try to render images with all three BRDFs):
    what differences do you see? Why?

    The lambertian BRDF have a weaker grazing retroreflection response than both diffuse and subsurface
    components. 
    Comparing the diffuse and the subsurface, although both have brighter grazing angle response, the
    subsurface one have even greater response. Also the brightness of subsurface is more evenly distributed
    across the sphere compared to the diffuse component.
	
	2. Play with the roughness parameter: how does it affect the appearance?

    With the roughness get higher, the appearance will generally get brighter.
    With the roughness get lower, the appearance will generally get darker.
    Specifically, the appearence differ the greatest at grazing angle, for example,
    the sphere will get bright arround its edge when roughness is high, 
    because the strong grazing retroreflection response.
	
	3. Compare the base diffuse BRDF (fbaseDiffuse) with the subsurface BRDF (fsubsurface) by playing with
    the subsurface parameter. What differences do you see? Why? In what lighting condition does
    the base diffuse BRDF differ the most from the subsurface BRDF? (Play with the light position in
    simple_sphere.xml for your experimentation)

    The more subsurface it is, the greater the difference between the grazing retroreflection of diffuse
    and subsurface. The subsurface is more uniform in color and have greater grazing retroreflection response.
    I found that diffuse and subsurface BRDF differs the most if only the grazing angle is not in shadow.
    This is because the grazing retroreflection differs the most.

    4. (Optional, bonus 3%) Another popular option for modeling diffuse surfaces is the Oren-Nayar BRDF [9],
    which is used in the Autodesk’s Standard Surface BSDF. What is the difference between the Oren-
    Nayar BRDF and the Disney diffuse BRDF? What are the pros and cons? What is your preferece

    Oren-Nayar BRDF treats the surface as micro lambertian surfaces. It does not consider internal material
    absorption and scattering like the Lommel-Seeliger Disney BRDF uses.
    Since Oren-Nayar is a micro facet model, it is more physically based. So it more physically accurate.
    However Disney BRDF is not as physically accurate as Oren-Nayar, since Lommel-Seeliger assumes a 
    smooth surface. Also Disney is a little "hacky" to make the results look more pleasing.
    My preference is Disney BRDF because is more artist friendly. We have more control over the result of
    the BRDF with roughness and subsurface.

2. Metal
    1. Compare DisneyMetal with the roughplastic material (try to render images with both BRDFs). What
    differences do you see? Why?

    The DisneyMetal is much more reflective than the roughplastic material. Metal is nearly mirror-like.
    Whereas the roughplastic behaves more like platic.
    This is because roughplastic material is the combination of diffuse and specular terms.
    However, DisneyMetal is a microfacet model that models the specular reflection since GGX 
    treats each microfacet as a mirror. Therefore DisneyMetal is much more reflective.

    2. Change the roughness parameters. Apart from how specular the surface it, do you observe any other
    differences?

    The surface also gets darker as the roughness gets higher. This is because we don't consider 
    multiple scattering in the Metal BRDF. This will lead to energy loss therefore change of color.

    3. A popular alternative over the Trowbridge-Reitz normal distribution function is the Beckmann distri-
    bution (a Gaussian distribution on the slopes hlz/hlx and hlz/hly of the normals). What are the differences
    between Trowbridge-Reitz and Beckmann? Why did Disney folks choose to use Trowbridge-Reitz in-
    stead of Beckmann? (You might want to read the awesome article Slope Space in BRDF Theory from
    Nathan Reed.)

    The Beckmann have a shorter tail than Trowbridge-Reitz.
    Since MERL metals have long tailed highlights, the Disney folks used the Trowbridge-Reitz
    to better fit the MERL dataset.

    4. (Optional, bonus 3%) What are the pros and cons of the Schlick approximation compared to the
    actual Fresnel equation? What is your preference? (You may want to read/watch the Naty Hoffman
    presentation in the footnote above.)

    My preferece is the Schlick approximation.
    The Schlick approximation is more computationally cheap.
    Also it is easier to implement and maintain.
    The approximation is also very good compared to the actual Fresnel.
    The actual Fresnel is more physically accurate, if we are using Spectrum it will be better
    than the approximation.

3. Clearcoat
    1. Compare DisneyClearcoat with DisneyMetal using similar roughness. What differences do you see?
    Where do the differences come from?

    At low Metal basecolor, the clearcoat have a more evenly distributed highlight, and the metal 
    is have a more concentrated highlight. I think this is because clearcoat have a longer tail 
    than metal.

    2. For coating, Autodesk Standard Surface uses a standard Trowbridge-Reitz microfacet distribution,
    instead of the modified normal distribution function proposed by Burley. What are the pros and cons?
    What is your preference?

    Autodesk uses the standard smith GGX so it have a shorter tail than Burley.
    Burley's method, although hacky, can model clearcoat with a longer tail hence more special cases.
    However I think Autodesk is better because it is more physically based and the end effect is very
    similar.

    3. Why do Burley limit the clearcoat BRDF to be isotropic?

    Because I think in real life anisotropic clearcoat BRDF is very rare, so it is not nessesary
    to add more complexity to this BRDF.

4. Glass
    1. Why do we take a square root of baseColor in the refractive case?

    Because we are accounting for two refractions. If we time them together we will get back 
    baseColor.

    2. Play with the index of refraction parameter η (the physically plausible range is [1, 2]). How does it
    affect appearance?

    A smaller eta will make the glass look more transparent and lighter in color.
    A bigger eta will make the glass refract light more and less transparent and darker in color.

    3. If a refractive object is not a closed object and we assign it to be a glass BSDF, will everything still
    work properly? Why? Propose a solution if it will not work properly (hint: you may want to read the
    thin-surface BSDF in Burley’s note [2].).

    If its not closed, then every ray passing through the object will think its still in the glass.
    Therefore its not going to work.
    To make it work we can model the open object as a very thin closed object. This approximation will 
    make the open objects work.

5. Sheen
    1. Render the simple_sphere scene with the sheen BRDF. What do you see? Why? What happens if you
    change the position of the light source?

    The scene would be totally black. If we change the light source to have some angle to our eye,
    we would observe the reflection. This is due to the brdf of sheen to model strong grazing angle
    reflection.

    2. Play with the parameter sheenTint, how does it affect the appearance? Why?

    As the sheenTint gets bigger, the reflection of the object gains more color and more saturation.
    Because we are doing (sheenTint · Ctint) so it will get more color and less white if we make
    sheenTint higher.

    3. In Autodesk Standard Surface, the sheen is modeled by a microfacet BRDF [4]. What are the pros
    and cons between the Autodesk approach and the Disney approach? What is your preference?

    I think the Autodesk version is better because is more physically based and looks more visually
    pleasing to me.
    However the disney version is easier to implement and also more easily controlable for the artist.

6. Putting it together
    1. What are the differences between the specular and metallic parameters? How do they affect the
    appearance?

    I think the main difference is that specular models the intensity of the specular reflections, while
    metallic model the how reflective an object is. For example objects with high specular will have a 
    brighter and broader highlight, whereas objects with high matallic will be more reflective of its
    surrounding therefore looks more metallic.

    2. What are the differences between the roughness and clearcoat_gloss parameters? How do they affect
    the appearance?

    Since the roughness is used by multiple BRDFs, changing it would result in dramatic changes to the
    specular and the general grazing response of the object. However clearcoat_gloss only controls the 
    specular highlight of the clearcoat BRDF, therefore it only affect the long tail highlight.

    3. Play with the specularTint parameter. How does it affect the appearance?

    The specular tint will affect the color and saturation of the specular highlight. similar to 
    the sheentint component.

    4. The roughness parameter affects many components of the BSDFs at once (e.g., both the diffuse and
    metal BRDF use the roughenss parameter). How do you feel about this? If you are an artist using the
    Disney BSDF, would you want to have a separate roughness parameter for each component?

    I think in a more physically based stand point this might be a bad idea. However if I am an artist,
    I would want this, because fewer parameters would be easier for me and makes more intuitive sense.
