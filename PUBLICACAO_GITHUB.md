# Publicação inicial no GitHub

Repositório sugerido: **oitaven/OpenDataLogger** (novo repositório público, vazio, sem README automático, licença ou .gitignore automático).

### Pela interface Web GitHub (mais simples)
1. Criar o repositório vazio em https://github.com/new — nome `OpenDataLogger`, visibilidade `Public`.
2. Abrir o repositório, escolher **uploading an existing file** / **Add file → Upload files**.
3. Descompactar este ZIP e arrastar para o GitHub o **conteúdo da pasta** `OpenDataLogger` (atenção: a interface Web pode não incluir ficheiros ocultos como `.github` ou `.gitignore` em alguns browsers).
4. Confirmar que NÃO está presente `arduino_secrets.h` (apenas `arduino_secrets.h.example`).
5. Confirmar commit e verificar a execução de `Actions → Compilar para Arduino UNO R4 WiFi`.

### Pelo Terminal no Mac (recomendado para os ficheiros ocultos)
```bash
cd /caminho/para/OpenDataLogger
git init -b main
git add .
git status
git commit -m "Initial public source: OpenDataLogger v1.0.2 (experimental)"
git remote add origin https://github.com/oitaven/OpenDataLogger.git
git push -u origin main
```

É necessário criar primeiro o repositório `oitaven/OpenDataLogger` e autenticar o `git` no Mac. Não executar `git add` na pasta Arduino habitual com `arduino_secrets.h`, salvo confirmação do `.gitignore`.

A licença incluída é **MIT**; rever se representa a intenção dos titulares de direitos antes de publicar. Revisitar o estado de validação no README após a primeira compilação real no GitHub Actions.
