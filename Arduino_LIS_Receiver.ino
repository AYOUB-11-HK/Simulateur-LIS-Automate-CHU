#include <SoftwareSerial.h>

// RX=10, TX=11 — reliés à D6 et D5 sur l'ESP8266
SoftwareSerial espSerial(10, 11);

void setup() {
  Serial.begin(9600);
  espSerial.begin(9600);

  Serial.println(F("================================================================"));
  Serial.println(F("       LIS — Laboratory Information System                      "));
  Serial.println(F("       Interface Réception Automate ASTM  |  v2.0               "));
  Serial.println(F("       Statut : EN LIGNE                                        "));
  Serial.println(F("================================================================"));
  Serial.println(F("En attente des resultats de l'Automate (via ESP8266)...\n"));
}

void loop() {
  // Attente d'un message de l'Automate (ESP8266 via SoftwareSerial)
  if (espSerial.available() > 0) {
    char c = espSerial.read();

    // Trame valide : commence par 'R' (Resultat)
    // Format : R|<ID>|<TYPE>|<VALEUR>
    if (c == 'R') {
      String trame = espSerial.readStringUntil('\n');
      trame.trim();

      // ── Parsing de la trame ASTM ──────────────────────────────
      int p1 = trame.indexOf('|');
      int p2 = trame.indexOf('|', p1 + 1);
      int p3 = trame.indexOf('|', p2 + 1);

      if (p1 != -1 && p2 != -1 && p3 != -1) {
        String echantillonID  = trame.substring(p1 + 1, p2);
        String typeAnalyse    = trame.substring(p2 + 1, p3);
        String valeurResultat = trame.substring(p3 + 1);

        // ── Affichage réception brute ─────────────────────────────
        Serial.println(F("\n>>> NOUVEAU MESSAGE ASTM RECU DE L'AUTOMATE <<<"));
        Serial.print(F("Trame brute : R"));
        Serial.println(trame);

        // ── Simulation validation biologique ─────────────────────
        delay(1000);
        Serial.println(F("... Validation biologique en cours ..."));
        delay(1500);

        // ── Résultat validé — prêt pour export HL7 ───────────────
        Serial.println(F("\n================================================================"));
        Serial.println(F("  RESULTAT VALIDE — PRET POUR EXPORT HL7 vers HIS             "));
        Serial.println(F("================================================================"));
        Serial.print(F("  ID Echantillon  : ")); Serial.println(echantillonID);
        Serial.print(F("  Type d'analyse  : ")); Serial.println(typeAnalyse);
        Serial.print(F("  Valeur          : ")); Serial.println(valeurResultat);
        Serial.println(F("  Statut          : VALIDE TECHNIQUEMENT"));
        Serial.println(F("  Destination     : HIS (via passerelle HL7)"));
        Serial.println(F("================================================================\n"));
        Serial.println(F("En attente de la prochaine analyse...\n"));

      } else {
        // Trame mal formée
        Serial.print(F("[ERREUR] Trame non conforme : R"));
        Serial.println(trame);
      }
    }
  }
}
