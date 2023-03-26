# Rendering Atmospheric Scattering with Volumetric Path Tracing
CSE272 WI23 Final project, Leo Cao  

time lapse             |  space view 
:-------------------------:|:-------------------------:
![](https://user-images.githubusercontent.com/49463679/227759202-ad77364c-cf2a-4d06-9843-90cbce4a82d4.gif)  |  ![](https://user-images.githubusercontent.com/49463679/227762243-1682d287-607f-4b49-89ec-0cac15f96da3.PNG)

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

  Rayleigh only             |  Rayleigh + Mie  |  Rayleigh only |  Rayleigh + Mie
:-------------------------:|:-------------------------:|:-------------------------:|:-------------------------:
![](https://user-images.githubusercontent.com/49463679/227763877-1095ff31-8c08-48d6-8fb6-756c0518ffc8.PNG)  |  ![](https://user-images.githubusercontent.com/49463679/227763882-852b8686-d1db-448f-8ab3-a81566d6dce1.PNG) | ![rayleigh_90](https://user-images.githubusercontent.com/49463679/227764056-2f16b976-0aac-48e6-bcf6-7d7259cd741d.PNG) | ![r_m_90](https://user-images.githubusercontent.com/49463679/227764061-8614fd1b-0a21-4396-a785-8796f73959e7.PNG)

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
  
## Importance Sampling

### Rayleigh
To importance sample the Rayleigh phase function, I implemented the inverse transform sampling method in "_Importance sampling the Rayleigh phase function_" by Frisvad.  
The math is a little weird, involves using Cardan’s formulas to solve a cubic function, but it comes down to this:  

$$ u = -[(2)(2\xi_1-1)+(4(2\xi_1-1)^2+1)^{1/2}]^{1/3} $$

$$ cos(\theta) = u - 1/u$$ 

$$ azimuth = 2\pi\xi_2 $$  

### Cornette-Shanks
Sampling methods for the Cornette-Shanks phase function is a little hard to find, but its very close to the HG phase function when $g = 0.72$, 
therefore I used the HG importance sampling method to approximate, which gave a pretty good result when testing. Reference is this paper[8] and Lajolla :)  

$$  cos(\theta) = \frac{1}{2g} (1+g^2-(\frac{1-g^2}{1-g+2g\xi_1})^2) $$  

$$ azimuth = 2\pi\xi_2 $$  

### Together
Since I assume the atmosphere is a even mixure of air molecules and water vapor, we can sample Rayleigh 50% of the time, and HG 50% of the time.  
And there pdfs would just be $0.5 * HG_{pdf} + 0.5 * rayleigh_{pdf}$

## Animation generation

To generate animation, I wrote a python script to read in and modify the xml data, then output hundreds of scene files.  
I also used the python script to render the scene files one by one to produce .exr images.  
In the end, I used DaVinci Resolve 8 to put the image sequence together to form a video. I used the built in Loop Up Tables to tone map the exr image. The LUT I used is "Linear to DMI".  
I rendered the transition from sunrise to sunset and also sealevel to space.
You can take a look at them on youtube:   
Time lapse: https://youtube.com/shorts/O2V1U5Pif4w  
Rayleigh only Time lapse: https://youtube.com/shorts/AC7zdxW4wqI  
From earth to space: https://youtube.com/shorts/Dkp57a4YYzA  

  Time lapse             |  Rayleigh only  |  earth to space
:-------------------------:|:-------------------------:|:-------------------------:
![](https://user-images.githubusercontent.com/49463679/227759202-ad77364c-cf2a-4d06-9843-90cbce4a82d4.gif)  |  ![](https://user-images.githubusercontent.com/49463679/227764996-2a32c9aa-9d1e-4aef-bde9-9377da6e40e9.gif) | ![](https://user-images.githubusercontent.com/49463679/227765061-231f477b-30c0-4380-b358-a29d72108324.gif) 

## Other results

### Belt of Venus
This effect of Venus Belt is observable after sunset. It is a pink and red belt, and it is located at the opposite side of the setting sun.  
This is caused by the backscatter of sunlight. The darker part at the bottom of the belt is caused by Earth's shadow.  

  Rendering            |  reference photo
:-------------------------:|:-------------------------:
![](https://user-images.githubusercontent.com/49463679/227765542-604e8615-d210-4d3a-9037-ddb202025ee5.PNG)  |  ![](https://user-images.githubusercontent.com/49463679/227765631-53f16088-3d6b-4da4-b6d2-f38f8ea37782.PNG)

### Aerial view

  Sunset aerial            |  earth from space
:-------------------------:|:-------------------------:
![](https://user-images.githubusercontent.com/49463679/227765708-9fae6414-c040-4dc7-92a8-2b626b7d9ba4.PNG)  |  ![](https://user-images.githubusercontent.com/49463679/227765772-17c584e1-0040-405a-875d-50fc35df175b.PNG)
  
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

[8]: https://arxiv.org/pdf/1812.00799.pdf
8: https://arxiv.org/pdf/1812.00799.pdf

