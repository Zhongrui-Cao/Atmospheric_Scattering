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
  In his paper, he modeled the atmosphere of the earth as 
  
  
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


