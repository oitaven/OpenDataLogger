# Protocolo ODLB v2 (OpenDataLogger 0.4.5)

Respostas de `GET /api/bin` em `application/octet-stream`, *little-endian*, `Content-Length` explícito.

O cabeçalho evoluiu de 48 para **64 bytes**; os primeiros 48 bytes preservam o significado da versão 1. Os valores são amostras ADC brutas; os flancos digitais são acontecimentos separados.

| Offset | Tipo | Campo |
|---:|---|---|
| 0 | ASCII 4B | `ODLB` |
| 4 | `u8` | `2` |
| 5 | `u8` | flags: bit 0 — reinício do histórico; bit 1 — transições perdidas pelo navegador |
| 6 | `u8` | canais ativos |
| 7 | `u8` | estado do trigger |
| 8 | `u16` | capacidade de amostras |
| 10 | `u16` | amostras disponíveis |
| 12 | `u32` | período ADC em ms |
| 16 | `u32` | instante do trigger em `millis()` |
| 20 | `u32` | geração lógica do histórico |
| 24 | `u32` | instante da amostra mais antiga |
| 28 | `u32` | instante da amostra mais recente |
| 32 | `u16` | amostras enviadas no corpo |
| 34 | `u16` | dimensão de cada amostra em bytes |
| 36 | `u8[8]` | IDs dos canais ativos |
| 44 | `u32` | contador total de amostras processadas |
| 48 | `u16` | flancos digitais enviados nesta resposta |
| 50 | `u32` | sequência mais recente de flancos existente no dispositivo |
| 54 | `u32` | contador de flancos substituídos na memória circular |
| 58 | `u8[6]` | reservado |

Depois do cabeçalho: `N` registos de amostra, cada um `u32 tempoMs` seguido de `u16` por canal ativo. Seguem-se `E` registos de flanco de **12 bytes**: `u32 seq`, `u32 tempoMs`, `u16 micros_fraccao`, `u8 id_canal`, `u8 nivel`.

Exemplo de consulta incremental:

`GET /api/bin?after=123400&gen=5&edgeAfter=86`

O navegador mantém cursores separados para amostras (`after`, `gen`) e transições (`edgeAfter`). Até **32 flancos** são enviados por resposta; os restantes vêm nas consultas seguintes. Se o anel de 128 transições substituir eventos não lidos, o bit 1 de flags assinala a lacuna. A versão 1 continua a ser descodificada pela página para compatibilidade, mas não inclui histórico de flancos.

Notas: `micros_fraccao` é uma referência submilissegundo **aproximada**; o relógio do R4 e a exactidão da IRQ têm de ser calibrados. O protocolo ainda não inclui autenticação, checksum de aplicação ou transporte seguro.

## OpenDataLogger 0.4.6

Nos bytes 58..61 do cabecalho de 64 bytes, anteriormente reservados, encontra-se agora `deviceNowMs` (`uint32_t` little-endian, tempo em milissegundos desde o arranque). O navegador usa-o como fim da janela de visualizacao, aproximando o eixo temporal da matriz LED. A matriz usa as amostras do historico GPT e os flancos digitais, em vez de um historico independente construido a partir de `latest[]`. A transferencia Web introduz uma latencia inevitavel.
