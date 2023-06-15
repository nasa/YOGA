#include "MeshQualityMetrics.h"
#include <parfait/DenseMatrix.h>
#include <parfait/Throw.h>
#include <parfait/ToString.h>
#include <functional>

namespace inf {
namespace MeshQuality {
    namespace HilbertCost {
        double jacobian(Parfait::Point<double> e1,
                        Parfait::Point<double> e2,
                        Parfait::Point<double> e3) {
            return Parfait::Point<double>::dot(e1, Parfait::Point<double>::cross(e2, e3));
        }
        double quad(const Parfait::Point<double>& p0,
                    const Parfait::Point<double>& p1,
                    const Parfait::Point<double>& p2,
                    const Parfait::Point<double>& p3) {
            Parfait::Point<double> e1, e2;
            double avg = 0.0;
            double min = 100;
            double j;
            double angle = 0;
            double total_angle_error = 0;
            double target_angle = 90;
            double angle_error = 0;
            double max_angle_allowed = 175;
            double max_angle = 0.0;
            double rad_to_deg = 180.0 / M_PI;
            // corner 0
            e1 = p1 - p0;
            e2 = p3 - p0;
            e1.normalize();
            e2.normalize();
            angle = acos(e1.dot(e2)) * rad_to_deg;
            angle_error = fabs(angle - target_angle);
            max_angle = std::max(max_angle, angle);
            total_angle_error += angle_error * angle_error;
            j = Parfait::Point<double>::cross(e1, e2).magnitude();
            min = std::min(j, min);
            avg += j;

            // corner 1
            e1 = p2 - p1;
            e2 = p0 - p1;
            e1.normalize();
            e2.normalize();
            angle = acos(e1.dot(e2)) * rad_to_deg;
            angle_error = fabs(angle - target_angle);
            max_angle = std::max(max_angle, angle);
            total_angle_error += angle_error * angle_error;
            j = Parfait::Point<double>::cross(e1, e2).magnitude();
            min = std::min(j, min);
            avg += j;

            // corner 2
            e1 = p3 - p2;
            e2 = p1 - p2;
            e1.normalize();
            e2.normalize();
            angle = acos(e1.dot(e2)) * rad_to_deg;
            max_angle = std::max(max_angle, angle);
            angle_error = fabs(angle - target_angle);
            total_angle_error += angle_error * angle_error;
            j = Parfait::Point<double>::cross(e1, e2).magnitude();
            min = std::min(j, min);
            avg += j;

            // corner 3
            e1 = p0 - p3;
            e2 = p2 - p3;
            e1.normalize();
            e2.normalize();
            angle = acos(e1.dot(e2)) * rad_to_deg;
            max_angle = std::max(max_angle, angle);
            angle_error = fabs(angle - target_angle);
            total_angle_error += angle_error * angle_error;
            j = Parfait::Point<double>::cross(e1, e2).magnitude();
            min = std::min(j, min);
            avg += j;

            (void)avg;
            if (min < 0.0) {
                return 1.0 - min;
            }
            if (max_angle > max_angle_allowed) {
                return max_angle / max_angle_allowed;
            }
            total_angle_error = sqrt(total_angle_error);
            double normalized_angle_error = total_angle_error / max_angle_allowed;
            return std::pow(
                normalized_angle_error,
                1.0 / 4.0);  // squish it so that bad angles are exagerated in cost space
        }
        double tri(const Parfait::Point<double>& p0,
                   const Parfait::Point<double>& p1,
                   const Parfait::Point<double>& p2) {
            Parfait::Point<double> e1, e2;
            double avg = 0.0;
            double min = 100;
            double j;
            // corner 0
            e1 = p1 - p0;
            e2 = p2 - p0;
            e1.normalize();
            e2.normalize();
            j = Parfait::Point<double>::cross(e1, e2).dot({0, 0, 1});
            min = std::min(j, min);
            avg += 1.0 / j;

            // corner 1
            e1 = p2 - p1;
            e2 = p0 - p1;
            e1.normalize();
            e2.normalize();
            j = Parfait::Point<double>::cross(e1, e2).dot({0, 0, 1});
            min = std::min(j, min);
            avg += 1.0 / j;

            // corner 2
            e1 = p0 - p2;
            e2 = p1 - p2;
            e1.normalize();
            e2.normalize();
            j = Parfait::Point<double>::cross(e1, e2).dot({0, 0, 1});
            min = std::min(j, min);
            avg += 1.0 / j;

            if (min < 0.0) {
                return 1.0 - min;
            }
            avg /= double(3.0);
            return 1 - 1.0 / avg;
        }
        double tet(const Parfait::Point<double>& p0,
                   const Parfait::Point<double>& p1,
                   const Parfait::Point<double>& p2,
                   const Parfait::Point<double>& p3) {
            Parfait::Point<double> e1, e2, e3;
            double avg = 0.0;
            double min = 100;
            double j, c;
            // corner 0
            e1 = p1 - p0;
            e2 = p2 - p0;
            e3 = p3 - p0;
            e1.normalize();
            e2.normalize();
            e3.normalize();
            j = jacobian(e1, e2, e3);
            if (j < 0)
                c = 1.0 - j;
            else
                c = 1.0 - 1.0 / CornerConditionNumber::isotropicTet(e1, e2, e3);
            avg += c;

            // corner 1
            e1 = p2 - p1;
            e2 = p0 - p1;
            e3 = p3 - p1;
            e1.normalize();
            e2.normalize();
            e3.normalize();
            j = jacobian(e1, e2, e3);
            if (j < 0)
                c = 1.0 - j;
            else
                c = 1.0 - 1.0 / CornerConditionNumber::isotropicTet(e1, e2, e3);
            avg += c;

            // corner 2
            e1 = p0 - p2;
            e2 = p1 - p2;
            e3 = p3 - p2;
            e1.normalize();
            e2.normalize();
            e3.normalize();
            j = jacobian(e1, e2, e3);
            if (j < 0)
                c = 1.0 - j;
            else
                c = 1.0 - 1.0 / CornerConditionNumber::isotropicTet(e1, e2, e3);
            min = std::min(j, min);
            avg += c;

            // corner 3
            e1 = p0 - p3;
            e2 = p2 - p3;
            e3 = p1 - p3;
            e1.normalize();
            e2.normalize();
            e3.normalize();
            j = jacobian(e1, e2, e3);
            j = jacobian(e1, e2, e3);
            if (j < 0)
                c = 1.0 - j;
            else
                c = 1 - 1.0 / CornerConditionNumber::isotropicTet(e1, e2, e3);
            min = std::min(j, min);
            avg += c;

            avg /= double(4.0);
            return avg;
        }

        double hex(const Parfait::Point<double>& p0,
                   const Parfait::Point<double>& p1,
                   const Parfait::Point<double>& p2,
                   const Parfait::Point<double>& p3,
                   const Parfait::Point<double>& p4,
                   const Parfait::Point<double>& p5,
                   const Parfait::Point<double>& p6,
                   const Parfait::Point<double>& p7) {
            Parfait::Point<double> e1, e2, e3;
            double avg = 0.0;
            double j, c;

            // corner 0
            e1 = p1 - p0;
            e2 = p3 - p0;
            e3 = p4 - p0;
            e1.normalize();
            e2.normalize();
            e3.normalize();
            j = jacobian(e1, e2, e3);
            if (j < 0)
                c = 1.0 - j;
            else
                c = 1 - 1.0 / CornerConditionNumber::hex(e1, e2, e3);
            avg += c;

            // corner 1
            e1 = p2 - p1;
            e2 = p0 - p1;
            e3 = p5 - p1;
            e1.normalize();
            e2.normalize();
            e3.normalize();
            j = jacobian(e1, e2, e3);
            if (j < 0)
                c = 1.0 - j;
            else
                c = 1 - 1.0 / CornerConditionNumber::hex(e1, e2, e3);
            avg += c;

            // corner 2
            e1 = p3 - p2;
            e2 = p1 - p2;
            e3 = p6 - p2;
            e1.normalize();
            e2.normalize();
            e3.normalize();
            j = jacobian(e1, e2, e3);
            if (j < 0)
                c = 1.0 - j;
            else
                c = 1 - 1.0 / CornerConditionNumber::hex(e1, e2, e3);
            avg += c;

            // corner 3
            e1 = p0 - p3;
            e2 = p2 - p3;
            e3 = p7 - p3;
            e1.normalize();
            e2.normalize();
            e3.normalize();
            j = jacobian(e1, e2, e3);
            if (j < 0)
                c = 1.0 - j;
            else
                c = 1 - 1.0 / CornerConditionNumber::hex(e1, e2, e3);
            avg += c;

            // corner 4
            e1 = p7 - p4;
            e2 = p5 - p4;
            e3 = p0 - p4;
            e1.normalize();
            e2.normalize();
            e3.normalize();
            j = jacobian(e1, e2, e3);
            if (j < 0)
                c = 1.0 - j;
            else
                c = 1 - 1.0 / CornerConditionNumber::hex(e1, e2, e3);
            avg += c;

            // corner 5
            e1 = p4 - p5;
            e2 = p6 - p5;
            e3 = p1 - p5;
            e1.normalize();
            e2.normalize();
            e3.normalize();
            j = jacobian(e1, e2, e3);
            if (j < 0)
                c = 1.0 - j;
            else
                c = 1 - 1.0 / CornerConditionNumber::hex(e1, e2, e3);
            avg += c;

            // corner 6
            e1 = p5 - p6;
            e2 = p7 - p6;
            e3 = p2 - p6;
            e1.normalize();
            e2.normalize();
            e3.normalize();
            j = jacobian(e1, e2, e3);
            if (j < 0)
                c = 1.0 - j;
            else
                c = 1 - 1.0 / CornerConditionNumber::hex(e1, e2, e3);
            avg += c;

            // corner 7
            e1 = p6 - p7;
            e2 = p4 - p7;
            e3 = p3 - p7;
            e1.normalize();
            e2.normalize();
            e3.normalize();
            j = jacobian(e1, e2, e3);
            if (j < 0)
                c = 1.0 - j;
            else
                c = 1 - 1.0 / CornerConditionNumber::hex(e1, e2, e3);
            avg += c;

            avg /= double(8.0);
            return avg;
        }
        double prism(const Parfait::Point<double>& p0,
                     const Parfait::Point<double>& p1,
                     const Parfait::Point<double>& p2,
                     const Parfait::Point<double>& p3,
                     const Parfait::Point<double>& p4,
                     const Parfait::Point<double>& p5) {
            Parfait::Point<double> e1, e2, e3;
            double avg = 0.0;
            double j, c;

            // corner 0
            e1 = p1 - p0;
            e2 = p2 - p0;
            e3 = p3 - p0;
            e1.normalize();
            e2.normalize();
            e3.normalize();
            j = jacobian(e1, e2, e3);
            if (j < 0)
                c = 1.0 - j;
            else
                c = 1.0 - 1.0 / CornerConditionNumber::prism(e1, e2, e3);
            avg += c;

            // corner 1
            e1 = p2 - p1;
            e2 = p0 - p1;
            e3 = p4 - p1;
            e1.normalize();
            e2.normalize();
            e3.normalize();
            j = jacobian(e1, e2, e3);
            if (j < 0)
                c = 1.0 - j;
            else
                c = 1.0 - 1.0 / CornerConditionNumber::prism(e1, e2, e3);
            avg += c;

            // corner 2
            e1 = p0 - p2;
            e2 = p1 - p2;
            e3 = p5 - p2;
            e1.normalize();
            e2.normalize();
            e3.normalize();
            j = jacobian(e1, e2, e3);
            if (j < 0)
                c = 1.0 - j;
            else
                c = 1.0 - 1.0 / CornerConditionNumber::prism(e1, e2, e3);
            avg += c;

            // corner 3
            e1 = p5 - p3;
            e2 = p4 - p3;
            e3 = p0 - p3;
            e1.normalize();
            e2.normalize();
            e3.normalize();
            j = jacobian(e1, e2, e3);
            if (j < 0)
                c = 1.0 - j;
            else
                c = 1.0 - 1.0 / CornerConditionNumber::prism(e1, e2, e3);
            avg += c;

            // corner 4
            e1 = p3 - p4;
            e2 = p5 - p4;
            e3 = p1 - p4;
            e1.normalize();
            e2.normalize();
            e3.normalize();
            j = jacobian(e1, e2, e3);
            if (j < 0)
                c = 1.0 - j;
            else
                c = 1.0 - 1.0 / CornerConditionNumber::prism(e1, e2, e3);
            avg += c;

            // corner 5
            e1 = p4 - p5;
            e2 = p3 - p5;
            e3 = p2 - p5;
            e1.normalize();
            e2.normalize();
            e3.normalize();
            j = jacobian(e1, e2, e3);
            if (j < 0)
                c = 1.0 - j;
            else
                c = 1.0 - 1.0 / CornerConditionNumber::prism(e1, e2, e3);
            avg += c;

            avg /= double(6.0);
            return avg;
        }
        double pyramid(const Parfait::Point<double>& p0,
                       const Parfait::Point<double>& p1,
                       const Parfait::Point<double>& p2,
                       const Parfait::Point<double>& p3,
                       const Parfait::Point<double>& p4) {
            Parfait::Point<double> e1, e2, e3;
            double avg = 0.0;
            double j, c;

            // corner 0
            e1 = p1 - p0;
            e2 = p3 - p0;
            e3 = p4 - p0;
            e1.normalize();
            e2.normalize();
            e3.normalize();
            j = jacobian(e1, e2, e3);
            if (j < 0)
                c = 1.0 - j;
            else
                c = 1 - 1.0 / CornerConditionNumber::pyramidBottomCorner(e1, e2, e3);
            avg += c;

            // corner 1
            e1 = p2 - p1;
            e2 = p0 - p1;
            e3 = p4 - p1;
            e1.normalize();
            e2.normalize();
            e3.normalize();
            j = jacobian(e1, e2, e3);
            if (j < 0)
                c = 1.0 - j;
            else
                c = 1 - 1.0 / CornerConditionNumber::pyramidBottomCorner(e1, e2, e3);
            avg += c;

            // corner 2
            e1 = p3 - p2;
            e2 = p1 - p2;
            e3 = p4 - p2;
            e1.normalize();
            e2.normalize();
            e3.normalize();
            j = jacobian(e1, e2, e3);
            if (j < 0)
                c = 1.0 - j;
            else
                c = 1 - 1.0 / CornerConditionNumber::pyramidBottomCorner(e1, e2, e3);
            avg += c;

            // corner 3
            e1 = p0 - p3;
            e2 = p2 - p3;
            e3 = p4 - p3;
            e1.normalize();
            e2.normalize();
            e3.normalize();
            j = jacobian(e1, e2, e3);
            if (j < 0)
                c = 1.0 - j;
            else
                c = 1 - 1.0 / CornerConditionNumber::pyramidBottomCorner(e1, e2, e3);
            avg += c;

            // corner 4-0
            e1 = p1 - p4;
            e2 = p0 - p4;
            e3 = p3 - p4;
            e1.normalize();
            e2.normalize();
            e3.normalize();
            j = jacobian(e1, e2, e3);
            if (j < 0)
                c = 1.0 - j;
            else
                c = 1 - 1.0 / CornerConditionNumber::pyramidTopCorner(e1, e2, e3);
            avg += c;

            // corner 4-1
            e1 = p2 - p4;
            e2 = p1 - p4;
            e3 = p0 - p4;
            e1.normalize();
            e2.normalize();
            e3.normalize();
            j = jacobian(e1, e2, e3);
            if (j < 0)
                c = 1.0 - j;
            else
                c = 1 - 1.0 / CornerConditionNumber::pyramidTopCorner(e1, e2, e3);
            avg += c;

            // corner 4-2
            e1 = p3 - p4;
            e2 = p2 - p4;
            e3 = p1 - p4;
            e1.normalize();
            e2.normalize();
            e3.normalize();
            j = jacobian(e1, e2, e3);
            if (j < 0)
                c = 1.0 - j;
            else
                c = 1 - 1.0 / CornerConditionNumber::pyramidTopCorner(e1, e2, e3);
            avg += c;

            // corner 4-3
            e1 = p0 - p4;
            e2 = p3 - p4;
            e3 = p2 - p4;
            e1.normalize();
            e2.normalize();
            e3.normalize();
            j = jacobian(e1, e2, e3);
            if (j < 0)
                c = 1.0 - j;
            else
                c = 1 - 1.0 / CornerConditionNumber::pyramidTopCorner(e1, e2, e3);
            avg += c;

            avg /= double(8.0);
            return avg;
        }
    }
    namespace CornerConditionNumber {
        double quad(Parfait::Point<double> e1, Parfait::Point<double> e2) {
            e1.normalize();
            e2.normalize();
            return Parfait::Point<double>::cross(e1, e2).magnitude();
        }
        double hex(Parfait::Point<double> e1,
                   Parfait::Point<double> e2,
                   Parfait::Point<double> e3) {
            e1.normalize();
            e2.normalize();
            e3.normalize();
            Parfait::DenseMatrix<double, 3, 3> A = {
                {e1[0], e2[0], e3[0]}, {e1[1], e2[1], e3[1]}, {e1[2], e2[2], e3[2]}};
            auto A_inv = Parfait::inverse(A);
            auto f = A.norm() * A_inv.norm();
            return f / 3.0;
        }
        double isotropicTet(Parfait::Point<double> e1,
                            Parfait::Point<double> e2,
                            Parfait::Point<double> e3) {
            e1.normalize();
            e2.normalize();
            e3.normalize();
            Parfait::DenseMatrix<double, 3, 3> A = {
                {e1[0], e2[0], e3[0]}, {e1[1], e2[1], e3[1]}, {e1[2], e2[2], e3[2]}};
            Parfait::DenseMatrix<double, 3, 3> W = {{1.0, 0.5, 0.5},
                                                    {0.0, sqrt(3.0) / 2.0, sqrt(3.0) / 6.0},
                                                    {0, 0, sqrt(2.0) / sqrt(3.0)}};
            auto W_inv = Parfait::inverse(W);
            auto A_inv = Parfait::inverse(A);
            auto f = (A * W_inv).norm() * (W * A_inv).norm();
            return f / 3.0;
        }
        double prism(Parfait::Point<double> e1,
                     Parfait::Point<double> e2,
                     Parfait::Point<double> e3) {
            e1.normalize();
            e2.normalize();
            e3.normalize();
            Parfait::DenseMatrix<double, 3, 3> A = {
                {e1[0], e2[0], e3[0]}, {e1[1], e2[1], e3[1]}, {e1[2], e2[2], e3[2]}};
            Parfait::DenseMatrix<double, 3, 3> W = {
                {1.0, 0.5, 0.0}, {0.0, sqrt(3.0) / 2.0, 0}, {0, 0, 1.0}};
            auto W_inv = Parfait::inverse(W);
            auto A_inv = Parfait::inverse(A);
            auto f = (A * W_inv).norm() * (W * A_inv).norm();
            return f / 3.0;
        }
        double pyramidBottomCorner(Parfait::Point<double> e1,
                                   Parfait::Point<double> e2,
                                   Parfait::Point<double> e3) {
            e1.normalize();
            e2.normalize();
            e3.normalize();
            Parfait::DenseMatrix<double, 3, 3> A = {
                {e1[0], e2[0], e3[0]}, {e1[1], e2[1], e3[1]}, {e1[2], e2[2], e3[2]}};
            Parfait::DenseMatrix<double, 3, 3> W = {
                {1.0, 0.0, sqrt(2.0) / 2.0}, {0.0, 1.0, sqrt(2.0) / 2.0}, {0, 0, 0.5}};
            auto W_inv = Parfait::inverse(W);
            auto A_inv = Parfait::inverse(A);
            auto f = (A * W_inv).norm() * (W * A_inv).norm();
            return f / 3.0;
        }
        double pyramidTopCorner(Parfait::Point<double> e1,
                                Parfait::Point<double> e2,
                                Parfait::Point<double> e3) {
            e1.normalize();
            e2.normalize();
            e3.normalize();
            Parfait::DenseMatrix<double, 3, 3> A = {
                {e1[0], e2[0], e3[0]}, {e1[1], e2[1], e3[1]}, {e1[2], e2[2], e3[2]}};

            double r = sqrt(2.0) / 2.0;
            Parfait::DenseMatrix<double, 3, 3> W = {{r, -r, -r}, {r, r, -r}, {-0.5, -0.5, -0.5}};
            auto W_inv = Parfait::inverse(W);
            auto A_inv = Parfait::inverse(A);
            auto f = (A * W_inv).norm() * (W * A_inv).norm();
            return f / 3.0;
        }
    }

    namespace ConditionNumber {
        double tri(Parfait::Point<double> p0,
                   Parfait::Point<double> p1,
                   Parfait::Point<double> p2) {
            double avg = 0.0;
            double min = 5.0;
            double current = 0.0;
            Parfait::Point<double> e1, e2;

            e1 = p1 - p0;
            e2 = p2 - p0;
            current = CornerConditionNumber::quad(e1, e2);
            min = std::min(min, current);
            avg += current;

            e1 = p2 - p1;
            e2 = p0 - p1;
            current = CornerConditionNumber::quad(e1, e2);
            min = std::min(min, current);
            avg += current;

            e1 = p0 - p2;
            e2 = p1 - p2;
            current = CornerConditionNumber::quad(e1, e2);
            min = std::min(min, current);
            avg += current;

            if (min < 0.0) {
                return min;
            }
            return avg / 3.0;
        }
        double quad(Parfait::Point<double> p0,
                    Parfait::Point<double> p1,
                    Parfait::Point<double> p2,
                    Parfait::Point<double> p3) {
            double avg = 0.0;
            double min = 5.0;
            double current = 0.0;
            Parfait::Point<double> e1, e2;

            e1 = p1 - p0;
            e2 = p3 - p0;
            current = CornerConditionNumber::quad(e1, e2);
            min = std::min(min, current);
            avg += current;

            e1 = p2 - p1;
            e2 = p0 - p1;
            current = CornerConditionNumber::quad(e1, e2);
            min = std::min(min, current);
            avg += current;

            e1 = p3 - p2;
            e2 = p1 - p2;
            current = CornerConditionNumber::quad(e1, e2);
            min = std::min(min, current);
            avg += current;

            e1 = p0 - p3;
            e2 = p2 - p3;
            current = CornerConditionNumber::quad(e1, e2);
            min = std::min(min, current);
            avg += current;

            if (min < 0.0) {
                return min;
            }
            return avg / 4.0;
        }
        double tet(Parfait::Point<double> p0,
                   Parfait::Point<double> p1,
                   Parfait::Point<double> p2,
                   Parfait::Point<double> p3) {
            double avg = 0.0;
            double min = 5.0;
            double current = 0.0;
            Parfait::Point<double> e1, e2, e3;

            e1 = p1 - p0;
            e2 = p2 - p0;
            e3 = p3 - p0;
            current = CornerConditionNumber::isotropicTet(e1, e2, e3);
            min = std::min(min, current);
            avg += current;

            e1 = p2 - p1;
            e2 = p0 - p1;
            e3 = p3 - p1;
            current = CornerConditionNumber::isotropicTet(e1, e2, e3);
            min = std::min(min, current);
            avg += current;

            e1 = p0 - p2;
            e2 = p1 - p2;
            e3 = p3 - p2;
            current = CornerConditionNumber::isotropicTet(e1, e2, e3);
            min = std::min(min, current);
            avg += current;

            e1 = p0 - p3;
            e2 = p2 - p3;
            e3 = p1 - p3;
            current = CornerConditionNumber::isotropicTet(e1, e2, e3);
            min = std::min(min, current);
            avg += current;

            if (min < 0) return min;
            return avg / 4.0;
        }
        double hex(Parfait::Point<double> p0,
                   Parfait::Point<double> p1,
                   Parfait::Point<double> p2,
                   Parfait::Point<double> p3,
                   Parfait::Point<double> p4,
                   Parfait::Point<double> p5,
                   Parfait::Point<double> p6,
                   Parfait::Point<double> p7) {
            double avg = 0.0;
            double min = 5.0;
            double current = 0.0;
            Parfait::Point<double> e1, e2, e3;

            e1 = p1 - p0;
            e2 = p3 - p0;
            e3 = p4 - p0;
            current = CornerConditionNumber::hex(e1, e2, e3);
            min = std::min(min, current);
            avg += current;

            e1 = p2 - p1;
            e2 = p0 - p1;
            e3 = p5 - p1;
            current = CornerConditionNumber::hex(e1, e2, e3);
            min = std::min(min, current);
            avg += current;

            e1 = p3 - p2;
            e2 = p1 - p2;
            e3 = p6 - p2;
            current = CornerConditionNumber::hex(e1, e2, e3);
            min = std::min(min, current);
            avg += current;

            e1 = p0 - p3;
            e2 = p2 - p3;
            e3 = p7 - p3;
            current = CornerConditionNumber::hex(e1, e2, e3);
            min = std::min(min, current);
            avg += current;

            e1 = p7 - p4;
            e2 = p5 - p4;
            e3 = p0 - p4;
            current = CornerConditionNumber::hex(e1, e2, e3);
            min = std::min(min, current);
            avg += current;

            e1 = p4 - p5;
            e2 = p6 - p5;
            e3 = p1 - p5;
            current = CornerConditionNumber::hex(e1, e2, e3);
            min = std::min(min, current);
            avg += current;

            e1 = p5 - p6;
            e2 = p7 - p6;
            e3 = p2 - p6;
            current = CornerConditionNumber::hex(e1, e2, e3);
            min = std::min(min, current);
            avg += current;

            e1 = p4 - p7;
            e2 = p6 - p7;
            e3 = p3 - p7;
            current = CornerConditionNumber::hex(e1, e2, e3);
            min = std::min(min, current);
            avg += current;

            if (min < 0.0) return min;
            return avg / 8.0;
        }
        double prism(Parfait::Point<double> p0,
                     Parfait::Point<double> p1,
                     Parfait::Point<double> p2,
                     Parfait::Point<double> p3,
                     Parfait::Point<double> p4,
                     Parfait::Point<double> p5) {
            double avg = 0.0;
            double min = 5.0;
            double current = 0.0;
            Parfait::Point<double> e1, e2, e3;

            e1 = p1 - p0;
            e2 = p2 - p0;
            e3 = p3 - p0;
            current = CornerConditionNumber::prism(e1, e2, e3);
            min = std::min(min, current);
            avg += current;

            e1 = p2 - p1;
            e2 = p0 - p1;
            e3 = p4 - p1;
            current = CornerConditionNumber::prism(e1, e2, e3);
            min = std::min(min, current);
            avg += current;

            e1 = p0 - p2;
            e2 = p1 - p2;
            e3 = p5 - p2;
            current = CornerConditionNumber::prism(e1, e2, e3);
            min = std::min(min, current);
            avg += current;

            e1 = p5 - p3;
            e2 = p4 - p3;
            e3 = p0 - p3;
            current = CornerConditionNumber::prism(e1, e2, e3);
            min = std::min(min, current);
            avg += current;

            e1 = p3 - p4;
            e2 = p5 - p4;
            e3 = p1 - p4;
            current = CornerConditionNumber::prism(e1, e2, e3);
            min = std::min(min, current);
            avg += current;

            e1 = p4 - p5;
            e2 = p3 - p5;
            e3 = p2 - p5;
            current = CornerConditionNumber::prism(e1, e2, e3);
            min = std::min(min, current);
            avg += current;

            if (min < 0.0) return min;
            return avg / 6.0;
        }
        double pyramid(Parfait::Point<double> p0,
                       Parfait::Point<double> p1,
                       Parfait::Point<double> p2,
                       Parfait::Point<double> p3,
                       Parfait::Point<double> p4) {
            double avg = 0.0;
            double min = 5.0;
            double current = 0.0;
            Parfait::Point<double> e1, e2, e3;

            e1 = p1 - p0;
            e2 = p3 - p0;
            e3 = p4 - p0;

            current = CornerConditionNumber::pyramidBottomCorner(e1, e2, e3);
            min = std::min(min, current);
            avg += current;

            e1 = p2 - p1;
            e2 = p0 - p1;
            e3 = p4 - p1;

            current = CornerConditionNumber::pyramidBottomCorner(e1, e2, e3);
            min = std::min(min, current);
            avg += current;

            e1 = p3 - p2;
            e2 = p1 - p2;
            e3 = p4 - p2;

            current = CornerConditionNumber::pyramidBottomCorner(e1, e2, e3);
            min = std::min(min, current);
            avg += current;

            e1 = p0 - p3;
            e2 = p2 - p3;
            e3 = p4 - p3;

            current = CornerConditionNumber::pyramidBottomCorner(e1, e2, e3);
            min = std::min(min, current);
            avg += current;

            e1 = p0 - p4;
            e2 = p1 - p4;
            e3 = p2 - p4;
            current = CornerConditionNumber::pyramidTopCorner(e1, e2, e3);
            min = std::min(min, current);
            avg += 0.5 * current;

            e1 = p2 - p4;
            e2 = p3 - p4;
            e3 = p0 - p4;
            current = CornerConditionNumber::pyramidTopCorner(e1, e2, e3);
            min = std::min(min, current);
            avg += 0.5 * current;

            if (min < 0.0) return min;
            return avg / 5.0;
        }
    }
    std::vector<double> cellConditionNumber(const MeshInterface& mesh) {
        std::vector<double> condition_number(mesh.cellCount());

        for (int c = 0; c < mesh.cellCount(); c++) {
            inf::Cell cell(mesh, c);
            auto points = cell.points();
            auto type = cell.type();
            switch (type) {
                case inf::MeshInterface::TRI_3:
                    condition_number[c] = ConditionNumber::tri(points[0], points[1], points[2]);
                    break;
                case inf::MeshInterface::QUAD_4:
                    condition_number[c] =
                        ConditionNumber::quad(points[0], points[1], points[2], points[3]);
                    break;
                case inf::MeshInterface::TETRA_4:
                    condition_number[c] =
                        ConditionNumber::tet(points[0], points[1], points[2], points[3]);
                    break;
                case inf::MeshInterface::PYRA_5:
                    condition_number[c] = ConditionNumber::pyramid(
                        points[0], points[1], points[2], points[3], points[4]);
                    break;
                case inf::MeshInterface::PENTA_6:
                    condition_number[c] = ConditionNumber::prism(
                        points[0], points[1], points[2], points[3], points[4], points[5]);
                    break;
                case inf::MeshInterface::HEXA_8:
                    condition_number[c] = ConditionNumber::hex(points[0],
                                                               points[1],
                                                               points[2],
                                                               points[3],
                                                               points[4],
                                                               points[5],
                                                               points[6],
                                                               points[7]);
                    break;
                default: {
                    PARFAIT_THROW("Unimplemented type encountered: " +
                                  inf::MeshInterface::cellTypeString(type));
                }
            }
        }

        return condition_number;
    }
    double nodeMaxHilbertCost(const MeshInterface& mesh,
                              const std::vector<std::vector<int>>& n2c,
                              int n) {
        double max_cost = -1.0;
        for (auto c : n2c[n]) {
            auto cell = inf::Cell(mesh, c);
            double cost = MeshQuality::cellHilbertCost(cell.type(), cell.points());
            cost = std::max(0.0, cost);
            max_cost = std::max(max_cost, cost);
        }
        return max_cost;
    }
    double nodeCost(const inf::MeshInterface& mesh,
                    const std::vector<std::vector<int>>& n2c,
                    int n,
                    std::function<double(const inf::MeshInterface&, int)> cell_cost_function) {
        double avg_cost = 0.0;
        int count = 0;
        for (auto c : n2c[n]) {
            double cost = cell_cost_function(mesh, c);
            cost = std::max(0.0, cost);
            avg_cost += sqrt(cost);
            count++;
        }
        return avg_cost / double(count);
    }
    double nodeHilbertCost(const inf::MeshInterface& mesh,
                           const std::vector<std::vector<int>>& n2c,
                           int n) {
        double avg_cost = 0.0;
        double max_cost = -1.0;
        double count = 0;
        for (auto c : n2c[n]) {
            auto cell = inf::Cell(mesh, c);
            double cost = MeshQuality::cellHilbertCost(cell.type(), cell.points());
            cost = std::max(0.0, cost);
            avg_cost += sqrt(cost);
            max_cost = std::max(max_cost, cost);
            count++;
        }
        avg_cost /= double(count);
        if (max_cost > 1.0) return max_cost;
        return avg_cost;
    }
    double cellHilbertCost(inf::MeshInterface::CellType type,
                           const std::vector<Parfait::Point<double>>& points) {
        switch (type) {
            case inf::MeshInterface::NODE:
                return 0.0;
            case inf::MeshInterface::BAR_2:
                return 0.0;
            case inf::MeshInterface::TRI_3:
                return HilbertCost::tri(points[0], points[1], points[2]);
            case inf::MeshInterface::QUAD_4:
                return HilbertCost::quad(points[0], points[1], points[2], points[3]);
            case inf::MeshInterface::TETRA_4:
                return HilbertCost::tet(points[0], points[1], points[2], points[3]);
            case inf::MeshInterface::PYRA_5:
                return HilbertCost::pyramid(points[0], points[1], points[2], points[3], points[4]);
            case inf::MeshInterface::PENTA_6:
                return HilbertCost::prism(
                    points[0], points[1], points[2], points[3], points[4], points[5]);
            case inf::MeshInterface::HEXA_8:
                return HilbertCost::hex(points[0],
                                        points[1],
                                        points[2],
                                        points[3],
                                        points[4],
                                        points[5],
                                        points[6],
                                        points[7]);
            default: {
                PARFAIT_THROW("Unimplemented type encountered: " +
                              inf::MeshInterface::cellTypeString(type));
            }
        }
    }
    double cellHilbertCost(const MeshInterface& mesh, int c) {
        inf::Cell cell(mesh, c);
        auto points = cell.points();
        auto type = cell.type();
        return cellHilbertCost(type, points);
    }
    std::vector<double> cellHilbertCost(const MeshInterface& mesh) {
        std::vector<double> cost(mesh.cellCount());

        for (int c = 0; c < mesh.cellCount(); c++) {
            cost[c] = cellHilbertCost(mesh, c);
        }
        return cost;
    }
    std::vector<double> nodeHilbertCost(const MeshInterface& mesh,
                                        const std::vector<std::vector<int>>& n2c) {
        std::vector<double> cost(mesh.nodeCount());

        for (int n = 0; n < mesh.nodeCount(); n++) {
            cost[n] = nodeHilbertCost(mesh, n2c, n);
        }
        return cost;
    }
    std::vector<double> cellFlattnessCost(const MeshInterface& mesh) {
        std::vector<double> cost(mesh.cellCount());

        for (int c = 0; c < mesh.cellCount(); c++) {
            cost[c] = Flattness::cell(mesh, c);
        }
        return cost;
    }
}
}
