#version 330 core

out vec4 FragColor;

in vec2 fragCoord; // From vertex shader, in range [0,1]

const int MAX_SIZE = 203;
uniform vec3 spheres[MAX_SIZE];
uniform float radii[MAX_SIZE];
uniform vec3 colors[MAX_SIZE];
uniform float smoothness[MAX_SIZE];
uniform float emission[MAX_SIZE];
uniform int size;
uniform vec3 cameraPos;
uniform vec3 camForward;
uniform vec3 camUp;
uniform vec3 camRight;
uniform vec2 resolution;
uniform int frameCount;
uniform sampler2D uPrevFrame;   // Previous accumulated result

float randomValue(inout uint state){
    state = state * 747796405u + 2891336453u;
    uint result = ((state >> ((state >> 28) + 4u)) ^ state) * 277803737u;
    result = (result >> 22) ^ result;
    return float(result) * (1/4294967295.0);
}
vec3 randPointSphere(inout uint state){
    vec3 pos;
    for (int i = 0; i < 10; i++){
        pos.x = 2*randomValue(state)-1;
        pos.y = 2*randomValue(state)-1;
        pos.z = 2*randomValue(state)-1;
        float mag = dot(pos,pos);
        if (mag < 1 && mag != 0){
            return pos / sqrt(mag);
        }
    }
    return vec3(1,0,0);
}
float sphereRayCollision(vec3 rayPos, vec3 rayDir, vec3 spherePos, float sphereRadius){
    float epsilon = 0.1;
    vec3 ray_pos = rayPos - spherePos;
    float d = dot(ray_pos, rayDir);

    float discriminant = d*d - dot(ray_pos, ray_pos) + sphereRadius*sphereRadius;

    if (discriminant == 0){
        float t = -d;
        if (t > epsilon){
            return t;
        }
    }
    if (discriminant > 0){
        float sq = sqrt(discriminant);
        float t = -d - sq;
        if (t > epsilon){
            return t;
        }
        t = -d + sq;
        if (t > epsilon){
            return t;
        }
    }
    return -1;
}

void main() {
    float aspectRatio = 16./10.;
    vec2 sceenCoord = vec2((2*fragCoord.x-1) * aspectRatio, 2*fragCoord.y-1);

    uvec2 pixel = uvec2(fragCoord.x * resolution.x, fragCoord.y * resolution.y * aspectRatio);
    uint state = pixel.x + pixel.y * uint(resolution.x) + uint(resolution.x*resolution.y*(frameCount%1400)+frameCount);

    vec3 totalColor = vec3(0,0,0);
    int samples = 25;
    int bounceLim = 12;
    int aa = 5;

    int aaCycle = 0;
    for (int s = 0; s < samples; s++) {
        float xi = float(aaCycle % aa);
        float yi = float(aaCycle) / float(aa);

        float ox = (xi + 0.5) / float(aa) - 0.5f; // Center of each subpixel grid cell
        float oy = (yi + 0.5) / float(aa) - 0.5f;

        ox /= resolution.x/2;
        oy /= resolution.y/2;

        vec2 coord = sceenCoord + vec2(ox, oy);

        vec3 pos = cameraPos;
        vec3 dir = camForward + camRight * coord.x + camUp * coord.y;
        dir = normalize(dir);
        vec3 color = vec3(1, 1, 1);


        for (int i = 0; i < bounceLim; i++) {
            float best_t = 1000000;
            int best_sphere = -1;
            for (int i = 0; i < size; i++) {
                float t = sphereRayCollision(pos, dir, spheres[i], radii[i]);
                if ((t > 0) && (t < best_t)) {
                    best_t = t;
                    best_sphere = i;
                }
            }

            if (best_sphere != -1) {
                pos += dir * best_t;
                vec3 normal = normalize(pos - spheres[best_sphere]);

                color *= colors[best_sphere];
                if (emission[best_sphere] > 0) {
                    color *= emission[best_sphere];
                    break;
                }

                vec3 random = normalize(randPointSphere(state)+normal);
                vec3 reflect = dir-normal*2*dot(dir, normal);
                dir = mix(random, reflect, smoothness[best_sphere]);
            }
            else if (dir.y < 0){
                float t = ((-500)-pos.y)/dir.y;
                if (t > 0.01 && t < 1000000){
                    pos += dir*t;

                    vec3 normal = vec3(0, 1, 0);

                    color *= vec3(0.9,0.9,0.9);

                    vec3 random = normalize(randPointSphere(state)+normal);
                    vec3 reflect = dir-normal*2*dot(dir, normal);
                    dir = mix(random, reflect, 0);
                } else {
                    color*=0;
                    break;
                    vec3 sky = vec3(0.5,0.7,0.9);
                    vec3 sunDir = vec3(-0.5, 1, 1);
                    vec3 sunColor = vec3(100, 70, 30);
                    float sunStrength = pow(max(dot(dir, sunDir)-0.5,0), 128);
                    sky += sunColor*sunStrength;
                    color *= sky;
                    break;
                }
            }
            else {
                color*=0;
                break;
                vec3 sky = vec3(0.5,0.7,0.9);
                vec3 sunDir = vec3(-0.5, 1, 1);
                vec3 sunColor = vec3(100, 70, 30);
                float sunStrength = pow(max(dot(dir, sunDir)-0.5,0), 128);
                sky += sunColor*sunStrength;
                color *= sky;
                break;
            }

            if (i == bounceLim-1){
                color *= 0;
            }
        }
        totalColor += color;
        aaCycle++;
        if (aaCycle >= aa*aa) aaCycle = 0;
    }
    totalColor /= samples;
    totalColor = sqrt(totalColor);

    // Read previous accumulated color
    vec3 prev = texture(uPrevFrame, fragCoord).rgb;

    // Exponential moving average accumulation
    float frame = float(frameCount);
    vec3 accum = mix(prev, totalColor, 1.0 / (frame + 1.0));

    FragColor = vec4(accum, 1.0);
}
