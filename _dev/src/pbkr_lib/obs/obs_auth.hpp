#pragma once
// obs_auth.hpp — Calcul de la chaine d'authentification obs-websocket v5.
//
// Algorithme (doc officielle obsproject/obs-websocket) :
//   secret          = base64( sha256( password + salt ) )
//   auth_response   = base64( sha256( secret + challenge ) )
//
// Ne depend que de Sha256 et Base64 (voir sha256.hpp / base64.hpp). Aucune connaissance
// du reseau ni du JSON : classe purement fonctionnelle, testable isolement.

#include "sha256.hpp"
#include "base64.hpp"
#include <string>

class ObsAuth {
public:
    static std::string computeAuthResponse(const std::string& password,
                                            const std::string& salt,
                                            const std::string& challenge) {
        std::string secretInput = password + salt;
        Sha256::Digest secretHash = Sha256::hash(secretInput);
        std::string secretBase64 = Base64::encode(secretHash.data(), secretHash.size());

        std::string responseInput = secretBase64 + challenge;
        Sha256::Digest responseHash = Sha256::hash(responseInput);
        return Base64::encode(responseHash.data(), responseHash.size());
    }
};