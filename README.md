# Rendering Atmospheric Scattering with Volumetric Path Tracing
CSE272 WI23 Final project, Leo Cao  
Under construction please don't grade yet   

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
  figure out the light intensity being scattered out. $P_r$ is given as:  
  
  $$ P_r(\theta) = 3/4 * (1+cos^2(\theta))$$
  
  We also need to know the scattering coefficient $\sigma_s$ of Rayleigh. It is given by:  
  
  $$ \sigma_s = \frac{8\pi^3(n^2-1)^2}{3N\lambda^4} * \rho(h) $$
  
  Where:  
  $n = 1.00029$ is the index of refraction of air.  
  $N = 2.504 * 10^25$ is the molecular number density of air.  
  $\lambda$ is the wavelength of light.  
  $\rho(h)$ is the density function, given before, as $e^{-h/H}$.  
  Notice how everything except the $\rho(h)$ is a constant, therefore we can precompute the scattering coefficient at sealevel, 
  then multiply it by the density function: $\sigma_s(h) = \sigma_s(h=0) * e^{-h/H}$. $H = 8.5km$ for air.  
  Since I am just using RGB for now, we can precompute the coefficient for the three color channels.  
  I am using:  
  $\sigma_s(\lambda=680nm) = 0.00519673173$ for red,  
  $\sigma_s(\lambda=550nm) = 0.01214269792$ for green,  
  $\sigma_s(\lambda=440nm) = 0.02964525861$ for blue. (all terms calculated in km)  
  
  ### Mie  
  Mie scattering is responsible for the water vapor, causing white glories around the sun.  
  Since the actual Mie is hard to implement, a few papers[6] approximate the Mie phase function with 
  the Cornette-Shanks phase function. The Mie phase function exhibites strong forward scattering for water vapors, 
  while still having backward scattering, which the Henyey Greenstein phase function could not approximate.[7]  
  
  The Cornette-Shanks phase function is given by:  
  
  $$ P_m(g, \theta) = 3/2 * \frac{(1-g^2)(1+cos^2(\theta))}{(2+g^2)(1+g^2-2gcos(\theta))^{3/2}} $$  
  
  Where $g$ is the term to determine forward or backward scattering. We will use $g = 0.76$ like in this paper.[6]  
  
  For the scattering term, we will do the same trick as before: $\sigma_s(h) = \sigma_s(h=0) * e^{-h/H}$ where $H = 1.2$ for water vapor.  
  However unlike Rayleigh, water vapor does absorb some light. We will use $\sigma_s / \sigma_e = 0.9$ to approximate $\sigma_a$ for fitting the 
  measurement data.[6]  
  
  
  
  
  
  
  
  
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

[7]: https://www.shadertoy.com/view/wlGBzh
7: https://www.shadertoy.com/view/wlGBzh

