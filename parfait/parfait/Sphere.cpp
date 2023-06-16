#include "Sphere.h"
#include "STLFactory.h"

Parfait::Sphere::Sphere(const Parfait::Point<double>& c, double r) : center(c), radius(r) {}
bool Parfait::Sphere::intersects(const Parfait::Point<double>& p) {
    double dist = (p - center).magnitude();
    return dist < radius;
}
bool Parfait::Sphere::intersects(const Parfait::Extent<double>& e) {
    auto p = e.clamp(center);
    return intersects(p);
}
std::vector<Parfait::Facet> Parfait::Sphere::facetize() const { return STLFactory::generateSphere(center, radius); }
void Parfait::Sphere::visualize(std::string filename) {
    auto facets = facetize();
    Parfait::STL::write(facets, filename);
}
Parfait::Sphere Parfait::boundingSphere(const std::vector<Parfait::Point<double>>& points) {
    Parfait::Point<double> center = points[0];
    double radius = 0.0001;

    for (int i = 0; i < 2; i++) {
        for (auto pos : points) {
            auto len = (pos - center).magnitude();
            if (len > radius) {
                double alpha = len / radius;
                double alphaSq = alpha * alpha;
                radius = 0.5 * (alpha + 1 / alpha) * radius;
                center = 0.5 * ((1 + 1 / alphaSq) * center + (1 - 1 / alphaSq) * pos);
            }
        }
    }

    for (auto pos : points) {
        auto delta = pos - center;
        auto len = delta.magnitude();
        if (len > radius) {
            radius = (radius + len) / 2.0;
            center = center + ((len - radius) / len * delta);
        }
    }

    return Sphere(center, radius);
}
Parfait::Sphere Parfait::boundingSphere(Parfait::Facet f) {
    Sphere s;
    // Calculate relative distances
    double A = (f[0] - f[1]).magnitude();
    double B = (f[1] - f[2]).magnitude();
    double C = (f[2] - f[0]).magnitude();

    // Re-orient triangle (make A longest side)
    const auto* a = &f[2];
    const auto* b = &f[0];
    const auto* c = &f[1];
    if (B < C) {
        std::swap(B, C);
        std::swap(b, c);
    }
    if (A < B) {
        std::swap(A, B);
        std::swap(a, b);
    }

    // If obtuse, just use longest diameter, otherwise circumscribe
    if ((B * B) + (C * C) <= (A * A)) {
        s.radius = A / 2.0;
        s.center = (*b + *c) / 2.0;
    } else {
        double cos_a = (B * B + C * C - A * A) / (B * C * 2);
        s.radius = A / (sqrt(1 - cos_a * cos_a) * 2.0);
        auto alpha = *a - *c;
        auto beta = *b - *c;
        auto axb = Parfait::Point<double>::cross(alpha, beta);
        auto ada = Parfait::Point<double>::dot(alpha, alpha);
        auto bdb = Parfait::Point<double>::dot(alpha, alpha);
        s.center =
            Parfait::Point<double>::cross(beta * ada - alpha * bdb, axb) / Parfait::Point<double>::dot(axb, axb * 2.0) +
            *c;
    }
    for (int i = 0; i < 3; i++) {
        double d = (f[i] - s.center).magnitude();
        if (d > s.radius) {
            s.radius = d;
        }
    }
    return s;
}
