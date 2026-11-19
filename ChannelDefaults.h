#pragma once
// OpenDataLogger 1.0.2: perfil inicial de canais.
// EDITAVEL pelo utilizador; so aplicado se a EEPROM nao contiver
// uma configuracao compativel, ou depois de repor os valores de fabrica.
// Nomes genericos; cada experiencia pode renomear os canais na pagina Web.
// Estrutura: nome, cor, tipo, autoScale, ganho, offset, min, max,
//            Rfixa, R25, beta, trim.
// Tipos: OFF=0, VOLTAGE=1, NTC=2, CURRENT=3, POWER=4,
//        CUSTOM=5, DIGITAL=6.
static const Channel ODL_DEFAULT_CHANNELS[CHANNELS] = {
  {"A0", "#60a5fa", VOLTAGE, 1, 1.0f, 0.0f, 0.0f, 5.0f, 4700.f, 4700.f, 3950.f, 0.f},
  {"A1", "#c084fc", VOLTAGE, 1, 1.0f, 0.0f, 0.0f, 5.0f, 4700.f, 4700.f, 3950.f, 0.f},
  {"A2", "#22c55e", OFF, 1, 1.0f, 0.0f, 0.0f, 5.0f, 4700.f, 4700.f, 3950.f, 0.f},
  {"A3", "#f97316", OFF, 1, 1.0f, 0.0f, 0.0f, 5.0f, 4700.f, 4700.f, 3950.f, 0.f},
  {"A4", "#06b6d4", OFF, 1, 1.0f, 0.0f, 0.0f, 5.0f, 4700.f, 4700.f, 3950.f, 0.f},
  {"A5", "#ec4899", OFF, 1, 1.0f, 0.0f, 0.0f, 5.0f, 4700.f, 4700.f, 3950.f, 0.f},
  {"D2", "#10b981", DIGITAL, 1, 1.0f, 0.0f, 0.0f, 5.0f, 4700.f, 4700.f, 3950.f, 0.f},
  {"D3", "#f59e0b", DIGITAL, 1, 1.0f, 0.0f, 0.0f, 5.0f, 4700.f, 4700.f, 3950.f, 0.f}
};
