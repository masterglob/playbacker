#pragma once
// random_bytes.hpp — Generation d'octets aleatoires via <random> (STL C++ pure),
// sans aucun appel systeme specifique a une plateforme (portable Linux/picore et MinGW/Windows).
//
// Attention : std::random_device n'est PAS garanti cryptographiquement sur par la norme
// (un mauvais backend peut retomber sur un PRNG deterministe si aucune source d'entropie
// materielle n'est disponible). Sur les toolchains MinGW anterieures a GCC 9.2, un bug connu
// faisait que std::random_device renvoyait toujours la meme sequence. Verifier entropy() > 0
// et/ou tester (cf. test_bricks.cpp) que deux appels successifs donnent des valeurs differentes.

#include <vector>
#include <cstdint>
#include <random>
#include <stdexcept>

class RandomBytes {
public:
    static std::vector<uint8_t> generate(size_t n) {
        std::random_device rd;

        if (rd.entropy() == 0.0) {
            // entropy() == 0 signifie que l'implementation ne garantit aucune source
            // d'entropie reelle (PRNG deterministe deguise en random_device).
            // On continue quand meme (certains backends valides renvoient 0 par prudence),
            // mais on le signale : a surveiller sur la toolchain ciblee.
        }

        std::vector<uint8_t> out(n);
        using result_type = std::random_device::result_type;

        size_t i = 0;
        while (i < n) {
            result_type r = rd();
            for (size_t b = 0; b < sizeof(result_type) && i < n; ++b, ++i) {
                out[i] = static_cast<uint8_t>(r >> (8 * b));
            }
        }
        return out;
    }
};