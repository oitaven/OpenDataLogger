# API Web local — OpenDataLogger 1.0.1 experimental

A API é servida por HTTP sem TLS na rede local. Não deve ser exposta à Internet. URLs e formatos estão sujeitos a alterações durante os testes.

| Endpoint | Uso |
|---|---|
| `GET /api/status` | Estado actual, canais, limites, configurações avançadas, disponibilidade SD e configuração de rede sem palavras-passe |
| `GET /api/diagnostics` | Intervalos, latência, FIFO, IRQ e contadores |
| `GET /api/bin?after=...` | Amostras e flancos incrementais em ODLB v2 (ver `PROTOCOL_ODLB_V2.md`) |
| `GET /api/data?after=...` | Histórico JSON legado |
| `GET /api/virtual?index=...` | Configuração de canal calculado (quatro no máximo) |
| `GET /api/alarm?index=...` | Configuração de regra de alarme (quatro no máximo) |
| `GET /api/alarmlog` | Últimos eventos de alarme |
| `GET /api/calpoint?channel=...` | Adicionar/limpar referência de calibração NTC |
| `GET /api/trigger?...` | Armar, parar e configurar trigger |
| `GET /api/channel?...` | Alterar configuração de entrada |
| `GET /api/rate?...` | Intervalo comum de aquisição |
| `GET /api/matrix?...` | Canal e janela temporal da matriz |
| `GET /api/sim?...` | Activar/desactivar simulação |
| `GET /api/save` | Guardar configurações persistentes suportadas |
| `POST /api/wifi` | Alterar redes e AP; exigir código `ODL_WEB_ADMIN_KEY`, guardar para próximo reinício |
| `GET /api/sd/status`, `/api/sd/list` | Disponibilidade e sessões SD |
| `GET /api/sd/start`, `/api/sd/stop` | Iniciar/parar sessão SD quando compilado e ligado |
| `GET /api/sd/download?name=...` | Descarregar ficheiro SD fechado |

**Atenção:** excepto o formulário Wi-Fi, os endpoints de configuração não exigem autenticação nesta versão. O próprio formulário Wi-Fi é transmitido sem cifragem de aplicação; utilizar somente em redes de confiança ou AP protegido. O microSD é detectado no arranque; gravação automática com cartão detectado, por omissão.

A análise de ciclos e canais calculados ocorre na interface Web com dados já adquiridos. As amostras analógicas brutas e as transições são prioritárias e não dependem dessas análises.
