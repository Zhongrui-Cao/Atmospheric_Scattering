# Rendering Atmospheric Scattering with Volumetric Path Tracing
CSE272 WI23 Final project, Leo Cao

![ezgif-3-a13a1bbbd6](https://user-images.githubusercontent.com/49463679/227759202-ad77364c-cf2a-4d06-9843-90cbce4a82d4.gif)

## Introduction

In this project, I built upon the volumetric path tracer we implemented in LaJolla
to render realistic atmospheric effects.  

I used physically based models and parameters for the earth, 
the sun, and the atmosphere. My implementation also accounts for multiple scattering and global illumination, which is important 
for physical rendering of sunset, sunrise, and the shadow of the Earth inside the atmosphere (the Belt of Venus effect).

## Physical Model and Parameters

  ### Astronomical Model
  I modeled every object in space to-scale.  
  The earth is modeled as a lambertian sphere with radius $Re = 6378 km$.[1]  
  If we are considering multiple scattering, the ground reflection of the sunlight into the atmosphere will actually make a difference in 
  the color of the sky. As discussed in papers like Nishita et al.'s "_Display method of the sky color taking into account multiple scattering_".[2]  
  Therefore, we sould also consider the albedo of the earth. Taken from this literature: "_The albedo of Earth_"[3], I used the albedo as 0.25 at 680nm for red, 
  0.25 at 550nm for green, and 0.35 at 440nm for blue.
  
  The sun is modeled as an emissive spherical light source.  
  For the radius of the Sun, I used $Rs = 695700 km$. The distance between the sun and the earth is $150000000km$. Both values taken from wikipedia.  
  The radiance of the Sun also varies based on the wavelength, I am using the radiance information in "_A Practical Analytic Model for Daylight_" by 
  Preetham et al.[4]
  
  ### Atmospheric Model
  To model the atmosphere of the earth, I took heavy inspiration from Nishita et al.'s paper "Display of the Earth Taking into Account Atmospheric Scattering".[5]  
  The atmosphere is a layer of gas surrounding the earth. So I am modeling it as a ball of participating media arround the earth.  
  The atmosphere is generally made up with air molecules(oxygen, nitrogen, argon), and water vapor/droplets.  
  The air molecules are responsible for the Rayleigh scattering, resulting in color of the blue sky. While the water in the atmosphere is responsible 
  for Mie scattering, resulting in the white "glare" we see arround the sun.  
  The atmosphere have different layers, each layer higher into space generally have exponentially smaller density. To simplify my model, I will assume the atmosphere 
  have only one layer, and the density of both air and water vapor decreases exponentially with height.  
  Therefore, we can model the density using this function: 
  
  $$ d(h) = d(0)*e^{-h/H} $$
  
  Where $d$ is the density at height $h$. $H$ is called the scale height, it is used in atomospheric sciences. It denote how thick would this media be on earth when the density is uniform.  
  The thickness of the atmosphere is usually defined by the Kármán line at $100km$.
  However, since most of the scattering happens in the troposphere and the stratosphere, 
  modeling the atmosphere with a thickness of $60km$ was shown to have better fitting to the measurement data. Denoted in "_Precomputed Atmospheric Scattering_" 
  by Bruneton et al.[6] 
  
## Phase functions and Implementation

  ### Rayleigh 
  Lets say we have a $\theta$ that denotes the angle between the incomming ray and the direction of light. We need to have a phase function $P$ in order to 
  figure out the light intensity being scattered out.  
  
  
## References
[1]: <https://en.wikipedia.org/wiki/Earth_radius>  
1: https://en.wikipedia.org/wiki/Earth_radius

[2]: <https://www.researchgate.net/publication/242513955_Display_method_of_the_sky_color_taking_into_account_multiple_scattering>
2: https://www.researchgate.net/publication/242513955_Display_method_of_the_sky_color_taking_into_account_multiple_scattering

[3]: <https://agupubs.onlinelibrary.wiley.com/doi/full/10.1002/2014RG000449>
3: https://agupubs.onlinelibrary.wiley.com/doi/full/10.1002/2014RG000449

[4]: https://courses.cs.duke.edu/fall01/cps124/resources/p91-preetham.pdf
4: https://courses.cs.duke.edu/fall01/cps124/resources/p91-preetham.pdf

[5]: http://nishitalab.org/user/nis/cdrom/sig93_nis.pdf
5: http://nishitalab.org/user/nis/cdrom/sig93_nis.pdf

[6]: https://hal.inria.fr/inria-00288758v1/document
6: https://hal.inria.fr/inria-00288758v1/document

