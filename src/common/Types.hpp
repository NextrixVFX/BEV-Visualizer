//
// Created by nextrix on 9/25/25.
//
#pragma once

struct Position {
    float x, y, z;
};

struct Size {
    float w, l, h;  // x, y, z
};

struct Velocity {
    float vx, vy;
};

struct BoundingBox {
    Position position;
    Size size;
    Velocity velocity;
    float z_rotation;
    float score;
    int id;
};