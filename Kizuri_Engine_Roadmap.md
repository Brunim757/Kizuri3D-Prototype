# Kizuri Engine — Roadmap de Desenvolvimento

**Versão:** 2.0 (direção de produção)
**Data:** Outubro de 2026
**Autor:** Kizuri Studio
**Repositório:** `KizuriEngine` (público)

---

## Direção atual

A engine é **genérica**, não pertence a um gênero. O primeiro jogo é **urbano**, no espírito de GTA/Just Cause: cidade contínua, carros, pedestres, trânsito, missões, clima e ciclo de dia/noite. Souls-like continua suportado como camada de gameplay, e cel shading é um modo de renderização de primeira classe.

O padrão de entrega muda: cada sistema precisa ser **de produção**, não apenas presente. Isso significa correção, testes, performance, ferramentas, documentação e acabamento visual. A referência de ambição é a RAGE: GTA V é DX11 e continua impressionante porque a engine por trás é madura.

A numeração 0–10 é histórica. A partir da Fase 10.5 o roadmap foi reorganizado para refletir a nova direção, sem apagar o que já foi concluído.

---

## Como ler este roadmap

As fases estão organizadas por **dependência técnica**, não por data — cada fase depende do que foi construído na anterior. Não há estimativa de tempo fixa (projeto solo, ritmo variável), mas a ordem deve ser respeitada: pular fases gera retrabalho.

Cada fase lista: **objetivo**, **entregáveis** e **critério de "pronto"**.

### Portões de qualidade (obrigatórios em toda fase)

1. **Ponta a ponta:** runtime, Editor, serialização, Undo/Redo, Inspector e persistência quando aplicável.
2. **Teste no CI:** unidade, integração e, quando gráfico, regressão visual; quando caro, orçamento de performance.
3. **Sem regressão conhecida:** nenhum bug P0/P1 no sistema tocado; artefato visual conhecido conta como bug.
4. **Performance medida:** frame time, memória, VRAM e carregamento contra a cena de referência.
5. **Ferramenta e debug:** configurável e inspecionável no Editor, com visualização quando fizer sentido.
6. **Documentação:** API, comportamento, limites e decisão de design registrados.
7. **Revisão de arquivos:** todos os arquivos tocados são relidos e verificados antes do commit.
8. **Um commit por correção verificada:** CI verde antes da próxima mudança.
9. **Sem placeholder, stub ou implementação simplificada.**
10. **Devlog em primeira pessoa:** estrutura antiga de "feito / próximo", começando direto em "corrigi", "removi", "melhorei", sem saudação artificial e sem nomes de arquivos internos.

### Princípio: o Editor nasce cedo e cresce junto com cada sistema

Uma casca de Editor (Fase 3) nasce logo após o primeiro pixel ser renderizado (Fase 2), antes de qualquer sistema de gameplay. A partir daí, **toda fase que introduz um componente novo no ECS é obrigada a entregar, na mesma fase, o desenho daquele componente no Inspector** — nunca adiado para uma fase de "polimento" posterior (regra 7 do TDD).

A Fase 3 foi deliberadamente construída como uma fundação robusta (não um MVP raso), porque sistemas essenciais de UX de editor (Undo/Redo, seleção múltipla, notificações, autosave) são muito mais caros de retrofitar depois do que de construir certo desde o início — o mesmo raciocínio já aplicado aos riscos técnicos da seção 8 do TDD.

### Princípio de integração de bibliotecas de terceiros

**Regra explícita**: uma biblioteca de terceiros só é integrada na fase em que o sistema que a usa é realmente construído — nunca antes. Integrar sem uso real produz apenas um link vazio, não testado de verdade.

**Tabela de mapeamento — qual fase integra cada biblioteca da stack (TDD, seção 3):**

| Biblioteca | Fase de integração | Motivo |
|---|---|---|
| DirectXMath | Fase 1 | Núcleo — matemática usada por tudo desde o início |
| enkiTS | Fase 1 | Núcleo — Job System é fundação de todo o resto |
| Dear ImGui | Fase 3 | Editor Shell — nasce logo após o primeiro pixel renderizado |
| cgltf | Fase 4 | Usada pelo Pipeline de Assets |
| stb_image | Fase 4 | Usada pelo Pipeline de Assets |
| DirectXTex | Fase 4 | Requer wiring específico com DirectXMath (CONFIG) |
| FlatBuffers | Fase 4 | Formato de serialização de assets |
| zstd | Fase 4 (bootstrap já validado na Fase 0) | Compressão de pacote de assets |
| Jolt Physics | Fase 6 | Sistema de física; aprofundado na Fase 13 |
| miniaudio | Fase 7 | Sistema de áudio; aprofundado na Fase 12 |
| nethost / hostfxr | Fase 14 | Tooling .NET para hospedar o CoreCLR |
| Roslyn (Microsoft.CodeAnalysis.CSharp) | Fase 14 | Compilador C# usado pelo scripting |
| Recast & Detour | Fase 18 | NavMesh e pathfinding |
| FreeType | Fase 21 | Renderização de texto (UI de jogo e/ou editor) |
| GameNetworkingSockets | Fase 27 | Rede P2P opcional; toolchain pesado (OpenSSL/libsodium), isolado até esta fase |
| Fork Awesome + IconFontCppHeaders | Fase 28 | Ícones do editor |

**Nota sobre a Fase 0**: libs single-header/simples de baixo risco foram todas *bootstrapadas* já na Fase 0 (CMake + chamada real de API via `KizuriHello`). `DirectXTex`, `GameNetworkingSockets`, `Roslyn`/`nethost` e os ícones seguem fora da Fase 0 pelos motivos já descritos na tabela.

---

## Status atual das fases

| Fase | Estado | Observação |
|---|---|---|
| 0 — Repositório | ✅ concluída | CI no GitHub Actions entrega artefato de build a cada push |
| 1 — Núcleo (C++) | ✅ concluída | Alocadores, Job System (enkiTS), janela/input, Origin Rebasing, RHI + backend DX11 |
| 2 — Renderer básico | ✅ concluída | Deferred + PBR + câmera livre + luz direcional. A lista de features do `RendererList.md` é ampla, mas a **qualidade de produção** ainda não estava no nível exigido; a Fase 11 revisa e corrige o pipeline inteiro (cor, sombras, TAA, materiais e pós) |
| 3 — Editor Shell | ✅ concluída | Viewport, Hierarquia, Inspector, Console, Asset Browser, Settings; serialização `.kzscene`, autosave, Undo/Redo, seleção múltipla, notificações, menus de contexto, ImGuizmo. **UI/UX no estilo Roblox Studio**: tema próprio (paleta + escala compacta + Segoe UI), dock space real com layout Explorer/Properties/Viewport/Output, barra de ferramentas, Hierarquia como árvore com busca, Inspector como property grid, Output com filtro. Layout salvo em `%LOCALAPPDATA%/Kizuri/EditorLayout.ini` |
| 4 — Pipeline de Assets | ✅ concluída | Import glTF, texturas, codecs internos, Asset Browser, dependências entre assets, upload assíncrono |
| 5 — Iluminação/sombras | ✅ concluída | CSM, PCF/PCSS, múltiplas luzes, clustered/Forward+, céu procedural, instancing, culling, FSR, Inspector de Light |
| 6 — Física (Jolt) | ✅ concluída | Jolt integrado, corpos/coliders, passo fixo 60Hz + interpolação, raycasts, ragdoll/joints, Inspector de Rigidbody, wireframe de colliders, toggle Simulate |
| 7 — Áudio + Mixer | ✅ concluída | Núcleo sobre miniaudio (device + decoder), mixer próprio com hierarquia de buses (Master/Música/SFX/Voz/Ambiente), volume/mute/solo, ducking entre buses, reverb (Freeverb) por bus, camada 3D posicional própria (atenuação linear/inverse/exponential + pan direcional), componente `AudioSource` no ECS (Undo/Redo + EditQueue + serialização + Inspector), painel "Audio Mixer" no Editor, mixer salvo como asset `.kzmixer`. 9 testes no CI |
| 8 — Runtime de Jogo | ✅ concluída | **A espinha**: `Engine/Game` com `GameLoop` (passo fixo determinístico + sistemas ordenados + estatísticas), `GameWorld` (cena + assets + renderer + física + áudio + câmera), `GameApp` (janela + RHI + loop) e o executável de jogo `KizuriRuntime.exe`. Material e luzes agora têm **uma implementação só** (Editor e runtime). 4 testes no CI |
| 9 — ECS e Gameplay Framework | ✅ concluída | `ComponentRegistry` é a fonte única de presença/defaults/reset/serialização/Undo/metadados de Inspector dos 11 componentes; Add/Remove e Save/Load são genéricos. O storage ECS usa colunas sparse-set, queries e scheduler por dependências; `SceneEcsBridge` preserva os IDs e o runtime executa física/áudio sobre ECS. O Play-in-Editor usa snapshot por raiz, cena isolada, `simEcs` e Inspector somente leitura. Validação visual concluída no E5700/GT 610. A migração 100% ECS (9.5) permanece opcional e não aprovada |
| 10 — Animação | 🟡 **em andamento** | Núcleo de esqueleto, poses, skinning, importação glTF, cache por malha/revisão, pool de ossos e componente Animator integrados ao Editor e ao runtime. Faltam state machine, blend trees, camadas, IK, root motion, eventos, retargeting, sincronização de hitbox e editor de animação |
| 10.5 — Estabilização crítica | ⏳ **próxima** | Corrigir componente Material, flicker/rachaduras de luz, viewport cinza no resize e tremulação residual do TAA; criar cenas de referência e baseline de performance |
| 11 — Renderização de produção | ⏳ pendente | Cor, sombras, iluminação, pós, materiais, cel shading, debug/profiler e regressão visual |
| 12 — Áudio de produção | ⏳ pendente | Mixer completo, DSP, spatialização, oclusão, snapshots, streaming, editor e testes |
| 13 — Física de produção | ⏳ pendente | CCD, substepping, joints, ragdoll, character controller, veículos, queries, debug e benchmarks |
| 14–33 | ⏳ pendente | Scripting C#, Prefabs, Scripting Visual, Câmera/Input, IA/Navegação, Save, VFX, UI, streaming, veículos, trânsito, clima, level design, rede opcional, editor, docs, launcher, build, vertical slice e endurecimento de produção |

**Onde a Fase 6 travou (e foi resolvido):** o teste de ragdoll derrubava o CI com `exit code 1` (access violation) — dois bugs reais no `Engine/Physics`:
1. *Double free* de constraint: no Jolt v5 `Constraint` é `RefTarget` e o `ConstraintManager` guarda `Array<Ref<Constraint>>`, logo `RemoveConstraint()` **já** libera o objeto; o `delete` manual do lado Kizuri destruía a mesma memória duas vezes. Agora o slot guarda `JPH::Ref<JPH::Constraint>` e quem controla a vida é o refcount do próprio Jolt.
2. *Use-after-free* no adaptador `Jolt::JobSystem → enkiTS`: tasks do enkiTS ainda pendentes na fila chamavam `ReleaseSlot()` num `JobSystemEnki` já destruído (o `WaitforAllAndShutdown()` do enkiTS executa essas tasks na thread principal). Agora o pool é compartilhado por `weak_ptr`, o destrutor drena os jobs em voo antes de liberar os slots, e o `PhysicsWorld` derruba o adaptador **antes** de destruir os corpos.

Lição operativa: **nunca chamar `delete` em objetos do Jolt v5** — o refcount do próprio Jolt é a fonte da verdade (`Ref<T>`).

---

## Fase 0 — Fundação do repositório

**Objetivo:** preparar o esqueleto do projeto antes de qualquer código de engine.

- Estrutura de pastas do repositório (`Engine/`, `Editor/`, `Launcher/`, `ThirdParty/`, `Samples/`)
- Configuração de build via GitHub Actions (compilação automática a cada push)
- **CI publica artefato de build baixável** (`actions/upload-artifact`) a cada execução — não só reporta verde/vermelho, entrega o binário compilado para o autor testar localmente (regras 5 e 8 do TDD); esse padrão vale para todo pipeline de CI daqui pra frente, não só a Fase 0
- Estrutura de solução C++ preparada para múltiplas `.lib` modulares desde o início
- Integração inicial de todas as bibliotecas de terceiros definidas no TDD (sem uso ainda, só linkando "hello world" de cada uma)

**Pronto quando:** o CI builda com sucesso um executável vazio que linka todas as `.lib` de terceiros, e esse executável fica disponível como artefato baixável na execução do workflow.

---

## Fase 1 — Núcleo de baixo nível (C++)

**Objetivo:** a fundação que tudo depende.

- Alocadores customizados (pool allocator, arena/stack allocator)
- Job System (integração do enkiTS)
- Janela e input via Win32 API nativo
- **Input crítico (esquiva, ataque) via polling de baixo nível (Raw Input/`GetAsyncKeyState`)**, fora do loop padrão de mensagens do Win32
- **Sistema de coordenadas com Origin Rebasing**: a origem do mundo é periodicamente reancorada perto do jogador, evitando erro de arredondamento de `float` 32-bit em mundos grandes
- RHI (Render Hardware Interface) — camada de abstração, ainda sem backend real
- Backend RHI para DirectX 11

**Pronto quando:** existe uma janela abrindo, um job rodando em paralelo à thread principal, e um `Clear()` de tela funcionando via RHI→DX11.

---

## Fase 2 — Renderer básico

**Objetivo:** primeira imagem 3D real na tela.

- Pipeline de Deferred Rendering
- **Cache de estado na RHI**: descarta chamadas de Render State já ativas na GPU
- Carregamento de mesh estática + material PBR simples
- Uma luz direcional funcional
- Câmera livre (debug, sem gameplay ainda)

**Pronto quando:** um modelo 3D importado aparece na tela, iluminado, com material PBR básico.

---

## Fase 3 — Editor Shell (fundação completa de UX)

**Objetivo:** dar ao dev uma forma robusta de ver e mexer no que já existe, antes de qualquer sistema de gameplay — incluindo os fundamentos de UX (Undo/Redo, seleção múltipla, notificações, autosave) que ficariam caros demais para adicionar depois.

**Painéis e infraestrutura base**
- Integração do Dear ImGui, tema dark
- Painéis: **Viewport**, **Hierarquia**, **Inspector** (com registro de "desenhadores" de componente, extensível), **Console**
- Gizmo de transformação (ImGuizmo) — modo Move, já que só existe Transform até aqui
- **Fila de edição no Inspector**: alterações da UI (Immediate Mode) são enfileiradas e aplicadas no início do frame seguinte, no mesmo ponto de sincronização do ECS
- Drag and drop básico (arrastar mesh já carregada para o Viewport cria entidade)
- Menu Bar inicial (File/Edit/Window/Help)
- **Menus de contexto (botão direito)**: Hierarquia (Criar Entity — sempre nasce só com Transform, sem redundância entre "vazio" e "cheio" já que adicionar qualquer outro componente é sempre um passo separado via Add Component ou drag-and-drop; Duplicar, Deletar, Renomear, Focar); Viewport (em objeto: Focar/Duplicar/Deletar; em espaço vazio: Criar Entity no ponto clicado); Inspector (Remover Componente, Resetar, Copiar/Colar valores) — cada painel novo em fases futuras ganha seu próprio menu de contexto na mesma entrega
- **Localização de shaders**: HLSL em pasta externa (`Shaders/`) durante o desenvolvimento da engine (hot-reload sem rebuild de C++); no `KizuriRuntime.exe` final (Fase 31), shaders são compilados para bytecode e embutidos, não distribuídos soltos

**Serialização de cena**
- **Serialização genérica baseada em reflection**: capacidade central de serializar qualquer componente para disco (`.kzscene`) e recarregar — "Salvar Cena" funciona de verdade desde aqui, não fica de fachada. Reaproveitada depois pela Fase 9 (snapshot de Play-in-Editor), Fase 15 (Prefabs) e Fase 19 (Save System do jogo) — mesma peça central, quatro consumidores
- **Autosave e recuperação de crash**: autosave periódico (intervalo configurável) para um arquivo de recuperação separado da cena principal; se o Editor abrir e encontrar uma recuperação mais recente que o último save, oferece restaurar
- **Indicador de alterações não salvas**: asterisco no título/aba da cena; prompt de confirmação ao fechar com mudanças pendentes

**Undo/Redo genérico**
- Sistema de Undo/Redo por **comando** (command pattern) e **diff incremental** (guarda só a mudança, não snapshot completo), cobrindo desde esta fase: criar/deletar entidade, adicionar/remover componente, editar valor no Inspector, mover/rotacionar/escalar via gizmo, parenting. A Fase 26 (Level Design) mais tarde **estende** este mesmo sistema para terreno/vegetação, em vez de implementar Undo/Redo do zero de novo

**Seleção múltipla**
- **Seleção por retângulo (rubber-band) no Viewport**: clicar e arrastar em espaço vazio desenha um retângulo; entidades cujo bounding box em tela intersecta o retângulo entram na seleção
- `Shift+clique` adiciona à seleção, `Ctrl+clique` alterna; mesma lógica na Hierarquia
- Gizmo em seleção múltipla: aparece na posição média do grupo; mover/rotacionar/escalar aplica o delta a cada entidade em relação à sua própria transform original (não colapsa todas num único pivô)

**Sistema de notificações (toasts)**
- Painel de notificações transitórias no canto da tela, com variantes de cor (erro/aviso/info/sucesso), auto-somem após alguns segundos, com um pequeno histórico recente acessível
- Usado sempre que uma ação não pode ser concluída (ex.: "Não é possível deletar: usado por 3 outros objetos", erro de build, falha de import) — complementa (não substitui) o sistema de validação contextual de componentes mal configurados (Fase 28)

**Pronto quando:** é possível criar entidades, selecionar várias por retângulo, mover o grupo pelo gizmo, desfazer/refazer qualquer uma dessas ações, salvar a cena (com indicador de não-salvo funcionando), e receber uma notificação clara quando uma ação for bloqueada — tudo sem editar código.

---

## Fase 4 — Pipeline de Assets

**Objetivo:** trazer conteúdo externo para dentro da engine de forma robusta.

- Import de glTF (cgltf)
- Import/compressão de texturas (stb_image + DirectXTex)
- Formato de asset interno via FlatBuffers (serialização própria, não glTF cru em runtime)
- Compressão de pacote de assets (zstd)
- **Upload assíncrono à GPU** via fila de staging buffers distribuída em vários frames
- **Texture Streaming por distância**: mip reduzido para objetos distantes
- **Asset Browser no Editor**: lista os assets importados; arrastar `.glb` para o Viewport cria entidade; arrastar para o campo Mesh no Inspector troca a malha (mapa caminho→índice, sem duplicar buffer na GPU)
- **Menu de contexto do Asset Browser**: Importar, Deletar, Renomear, Mostrar na pasta do sistema
- **Rastreamento de dependência entre assets**: cada asset mantém contagem de quem o referencia (ex.: um Material usando uma Textura); tentar deletar um asset em uso dispara uma notificação (Fase 3) listando os dependentes e bloqueia a exclusão, a menos que o dev confirme "forçar exclusão" — nesse caso, os dependentes passam a exibir um estado de "asset ausente" visível (não uma falha silenciosa)

**Pronto quando:** um `.glb` externo é importado, aparece no Asset Browser, pode ser arrastado para Viewport/Inspector, e tentar deletar um asset em uso gera aviso em vez de quebrar silenciosamente outra coisa.

---

## Fase 5 — Sombras e iluminação avançada

**Objetivo:** iluminação e sombras de nível production-ready para cidade e combate. A Fase 11 revisa a qualidade e a estabilidade do que foi construído aqui.

- Cascaded Shadow Maps (CSM) para luz direcional
- Shadow maps para luzes pontuais/spot
- Suavização (PCF/PCSS)
- Múltiplas luzes dinâmicas simultâneas
- Integração do FSR (upscaling)
- Céu procedural atrelado à luz direcional (gradiente + sol), reconstruído via depth + matriz inversa de view-projection
- **Instancing agressivo** e **culling de oclusão obrigatório** — não é otimização a ser adicionada depois
- **Inspector**: desenhador de componente **Light** (tipo, cor, intensidade, raio)

**Pronto quando:** uma cena com 10+ luzes dinâmicas e sombras suaves roda sem queda perceptível de frame, e uma luz pode ser criada/ajustada inteiramente pelo Inspector.

---

## Fase 6 — Física (Jolt)

**Objetivo:** colisão e simulação física básica.

- Integração do Jolt Physics
- **Adaptador `Jolt::JobSystem` → enkiTS**: evita guerra de threads entre os dois schedulers
- Rigidbody, colliders (box, capsule, mesh)
- Raycasts
- Ragdoll e joints configuráveis
- **Interpolação entre passos de física** para evitar tremor visual
- **Ordem de cálculo em plataformas móveis** para evitar atravessar chão/teto
- **Inspector**: desenhador de **Rigidbody** e **Ragdoll/Joints**; visualização de colliders no Viewport (wireframe)

**Pronto quando:** um personagem cai por gravidade, colide com o cenário, um ragdoll reage a uma força, e um Rigidbody pode ser configurado inteiramente pelo Inspector.

---

## Fase 7 — Áudio (com mixagem completa)

**Objetivo:** som posicional funcional e uma cadeia de mixagem de nível profissional, não apenas tocar clipes.

- Integração do miniaudio
- Camada de áudio 3D posicional própria (atenuação por distância, direção)
- **Audio Mixer como asset próprio**: hierarquia de buses (Master → Música, SFX, Voz, Ambiente), cada bus com volume, mute e solo independentes
- **Ducking configurável**: um bus pode abaixar automaticamente o volume de outro quando ativo (ex.: diálogo abaixa a música)
- **Efeitos por bus**: zonas de reverberação (ex.: caverna, salão de boss) aplicadas ao bus correspondente
- **Editor de Audio Mixer**: painel dedicado, editando buses, volumes e roteamento visualmente
- **Inspector**: desenhador de **AudioSource** (clipe, volume, raio de atenuação, loop, **bus de saída** via dropdown)

**Pronto quando:** uma fonte sonora 3D muda de volume/direção com a câmera, pode ser roteada para um bus específico, o volume master/por-bus é ajustável pelo Editor de Mixer, e tudo isso é configurável sem código.

---

## Fase 8 — Runtime de Jogo (a espinha)

**Objetivo:** transformar um conjunto de sistemas em **um jogo**. Até aqui a engine tinha renderer, editor, física e áudio — mas nada que rodasse um jogo de verdade. Esta fase entrega a espinha: um executável, um loop de passo fixo determinístico e um mundo onde tudo isso se conecta. Tudo o que vier depois (animação, scripting, ECS, prefabs) é **sistema** e se pluga aqui.

- **Módulo `Engine/Game`** (novo), compartilhado entre o Editor e o jogo:
  - `GameLoop` — passo fixo (60Hz) com acumulador e limite de passos (sem spiral of death), `alpha` de interpolação, pausa, quit, **registro ordenado de sistemas** (update/render) e estatísticas por sistema
  - `GameWorld` — o mundo do jogo: cena + assets + renderer + física + áudio + câmera; carrega `.kzscene`, sincroniza a física com a cena e monta o caminho de render de runtime
  - `GameApp` — janela + RHI + mundo + loop; input, resize, ESC sai
  - `GameMaterial` — a conversão material→renderer em **uma implementação só** (o Editor e o runtime usam a mesma; o mesmo vale para a submissão de luzes)
- **`KizuriRuntime.exe`** (o executável de jogo): abre uma cena (ou a cena padrão) e roda o loop — é o ancestral do executável base que o sistema de Build da Fase 31 vai empacotar
- **Caminho de render de runtime**: luzes da cena + meshes + materiais + câmera, sem as ferramentas de edição (gizmos, overlays de debug, batching estático do editor)
- **Entrada de runtime**: WASD move a câmera, mouse olha, ESC fecha

**Pronto quando:** `KizuriRuntime.exe` abre uma cena e roda com passo fixo, a câmera responde ao input, a física simula e o áudio toca — e o Editor e o runtime compartilham o **mesmo** mundo, o mesmo material e as mesmas luzes (fonte única de verdade). O Play-in-Editor (rodar o `GameLoop` dentro do Editor) faz parte da Fase 9.

---

## Fase 9 — ECS e Gameplay Framework

**Objetivo:** a camada que conecta o núcleo C++ à experiência do dev em C# — e a base sobre a qual Animação, Prefabs, IA e Scripting Visual são construídos.

- Estrutura ECS por baixo (cache-friendly, integrada ao Job System). O `Entity` atual (struct com flags `has*`) continua sendo a representação serializada; o storage ECS organiza os mesmos dados em arrays por componente
- **Escalonamento de leitura/escrita por componente**, evitando condição de corrida entre sistemas
- API C# que expõe o ECS como "componentes tradicionais"
- Sistema de Entity/parenting/hierarquia de cena
- **`ComponentRegistry` como fonte única**: presença, defaults, reset, serialização e metadados de Inspector num só lugar — adicionar componente novo não exige tocar no Editor nem no serializador
- **Play-in-Editor via snapshot em memória**, reaproveitando a serialização da Fase 3 e o `GameLoop`/`GameWorld` da Fase 8
- **Inspector/Hierarquia**: property grid gerado a partir do registro (sem código por componente); "Add Component" genérico (lista todos os componentes já registrados); parenting por drag-and-drop

**Pronto quando:** é possível criar uma Entity, adicionar qualquer componente via "Add Component", organizar em hierarquia por drag-and-drop, e testar com Play-in-Editor — e um componente novo aparece no Inspector apenas por ser registrado.

**Estado atual:** a ponte cena↔ECS é o `SceneEcsBridge`: a cena continua sendo a fonte de autoria e serialização; o ECS é importado no Play-in-Editor e no runtime, e os resultados de física/áudio são exportados de volta. O Play-in-Editor usa snapshot por raiz, cena isolada, `simEcs` e sistemas de física/áudio; o Inspector fica somente leitura. Os componentes com editores especializados (Light, Rigidbody, AudioSource, Probe, LOD, Lightmap) mantêm o desenho dedicado; os demais usam o property grid genérico. A validação visual de Play/DnD/property grid foi concluída no E5700/GT 610, junto com a correção do upload de modelos grandes, do menu de contexto da Hierarquia, do alinhamento da barra superior e da importação de clipes de áudio.

### Fase 9.5 — ECS como fonte única (proposta, não aprovada)

**Objetivo:** eliminar a ida e volta cena→ECS→cena e fazer o ECS ser a única fonte de verdade em edição e runtime, deixando a `Scene` apenas como formato de serialização/importação. É uma refatoração transversal (Hierarquia, Inspector, Undo/Redo, renderização, física, áudio, Play-in-Editor e Prefabs), não uma feature isolada.

- Transformar `Scene` em adaptador de I/O (`.kzscene` ↔ ECS), sem armazenar componentes duplicados
- Fazer Hierarquia, Inspector, seleção e Undo/Redo operarem diretamente no ECS
- Fazer a renderização percorrer o ECS (ou um snapshot imutável dele) em vez da `Scene`
- Unificar Play-in-Editor e runtime no mesmo mundo ECS, sem exportar transforms de volta
- Migrar Prefabs (Fase 15) para instanciar diretamente no ECS

**Por que agora:** Animação (Fase 10) e Prefabs (Fase 15) vão criar novos componentes e hierarquias; fazer a migração antes evita reescrever esses sistemas duas vezes. **Por que não bloquear a Fase 9:** o escopo aprovado da Fase 9 é a ponte cena↔ECS, e a arquitetura híbrida atual já entrega autoria, serialização, Play-in-Editor e runtime. A Fase 9.5 só deve começar se houver decisão explícita de trocar a fonte de verdade.

---

## Fase 10 — Animação (com retargeting)

**Objetivo:** animação de produção para personagens, multidões, veículos e combate — base tanto para a cidade quanto para um souls-like.

**Já entregue:**
- Núcleo de esqueleto, poses, skinning por GPU e amostragem de clipes glTF
- Importação de malhas com skin, cache de animação por malha e revisão de dados
- Conversão de CUBICSPLINE para linear a 60 Hz no import; STEP preservado
- Runtime com estado por entidade, `bone = IBM * model`, pool de ossos por frame e upload único antes das sombras
- Componente Animator no Inspector e integração com Editor/Play-in-Editor/runtime
- Malhas skinned corretamente excluídas de meshlets, LOD bake, instancing, GPU culling e static batching
- Testes de runtime de animação no CI

**Pendente:**
- State machine, blend trees, camadas, máscaras, additive, sync groups e eventos
- IK de pés no terreno, mãos em objetos, look-at de cabeça/tronco e ajuste de coluna
- Root motion com extração e aplicação corretas
- Retargeting para rig humanoide padrão
- Sincronização de hitbox no mesmo passo da animação
- Blend com ragdoll e animação de multidão
- Editor de state machine, timeline, preview, debugger de esqueleto/pesos e compressão/streaming de clipes

**Pronto quando:** um personagem anda sobre terreno irregular com IK correto, executa ataque com root motion preciso, a state machine é editável pelo Editor, uma animação é reaproveitada num segundo esqueleto via retargeting, a hitbox acompanha o osso no mesmo frame e não há popping entre clipes.

**Estado atual:** o núcleo de skinning e Animator está integrado e testado. A fase continua com os sistemas avançados acima antes de ser considerada concluída.

---

## Fase 10.5 — Estabilização crítica e revisão visual

**Objetivo:** eliminar as regressões visíveis antes de expandir o renderer e criar a baseline de qualidade que as fases seguintes vão usar.

- Corrigir o componente **Material** quando adicionado a uma entidade (modelo quebrado, texturas, alpha, flags ou preenchimento inicial)
- Corrigir **flicker de luz e rachaduras de sombra** na origem: bias, precisão, cascatas, normal offset, cache e estabilidade temporal
- Corrigir **viewport cinza ao redimensionar abas/docks**, incluindo recriação de render targets e tamanho zero transitório
- Reduzir a tremulação residual do **TAA**, com validação em câmera parada, em movimento e com objetos animados
- Auditar o estado real do renderer: cor, exposição, sombras, pós-processamento, materiais, TAA/velocity e debug views
- Criar cenas de referência para material, iluminação, cidade, personagem e veículo
- Registrar baseline de frame time, VRAM e memória no hardware de teste
- Criar o harness de regressão visual e performance usado pelas próximas fases

**Pronto quando:** os quatro problemas estão corrigidos com teste de regressão, as cenas de referência existem, a baseline está registrada e não há bug P0/P1 conhecido no renderer.

---

## Fase 11 — Renderização de produção

**Objetivo:** transformar o renderer de "conjunto de efeitos" em pipeline de produção: imagem estável, cor consistente, sombras sem artefatos, materiais corretos e custo previsível.

- **Cor e exposição:** workflow linear, sRGB correto, HDR, exposição manual/automática, tonemap ACES e alternativas, LUT, contraste, saturação, balanço de branco e consistência total entre Editor e runtime
- **Sombras:** CSM com cascatas estáveis, bias automático, normal offset, PCF/PCSS, contact shadows, point/spot, atlas, cache, resolução dinâmica com critério e eliminação de shimmer/rachaduras
- **Iluminação:** direcional, point, spot, area/rect, IBL, reflection probes, DDGI, light layers, cookies, unidades físicas, temperatura de cor, clustered/Forward+ e muitas luzes sem queda de frame
- **Materiais:** PBR correto em energia/Fresnel/normais/ORM/alpha/double-sided, clearcoat, sheen, transmission, volume, anisotropia, iridescência, dispersão, layered materials, decals, detail maps, POM, tessellation e virtual textures; corrigir o componente Material e o preenchimento a partir de malha
- **Pós-processamento:** SSAO/GTAO, SSR, SSGI, TAA com velocity buffer, FXAA, bloom, DOF, motion blur, god rays, fog volumétrico, film grain, vinheta e heat haze; ordem, qualidade e custo documentados
- **Geometria:** meshlets, GPU culling, oclusão, LOD, instancing, static batching e streaming; malhas skinned fora de meshlets, LOD bake, instancing, GPU culling e static batching
- **Cel shading:** rampa, contornos por normal/depth, especular estilizado, sombras pintadas, presets e convivência com PBR na mesma cena
- **Ferramentas:** render graph debugger, debug views, visualização de G-buffer/sombras/luzes/probes/overdraw/HZB, tempos de GPU por passe, shader hot reload e captura de frame
- **Qualidade obrigatória:** zero flicker de luz, zero rachadura de sombra, zero costura de cascata, zero ghosting de TAA, zero popping de LOD e zero diferença de cor entre Editor e jogo
- **Cenas de referência:** material, iluminação, cidade, personagem, veículo e efeitos; regressão visual e orçamento de frame time no CI

**Pronto quando:** as cenas de referência não apresentam artefatos, o preset alto roda a 1080p/60 no hardware recomendado, o piso GT 610 roda o preset baixo em 720p/30, e a regressão visual/performance está automatizada.

---

## Fase 12 — Áudio de produção

**Objetivo:** mixer e áudio espacial de nível profissional, não apenas tocar clipes.

- **Mixer:** grafo de buses hierárquico (Master → Música, SFX, Voz, Ambiente, UI, Veículos), sends/returns, inserts, volume, mute, solo e snapshots
- **DSP:** EQ, compressor, limiter, reverb, delay, filtros, distortion e medidores por bus
- **Ducking:** sidechain configurável entre buses, com ataque/release e prioridade
- **Espacialização:** atenuação, panning, Doppler, oclusão, reverb zones, ambiência e emitters 3D
- **Streaming e vozes:** clipes longos, voice management, prioridade, limite de vozes e virtualização
- **Editor:** Audio Mixer completo, roteamento visual, curvas, preview, salvamento como asset, Undo/Redo e Inspector de AudioSource
- **Qualidade obrigatória:** sem clique/estalo, sem corte abrupto, sem vazamento entre buses, latência e custo medidos
- **Testes:** roteamento, ducking, automação, sinal sintético, limite de vozes e performance

**Pronto quando:** uma cena com música, SFX, voz, ambiente e veículo mixa corretamente, o jogador ouve direção/distância/oclusão, o mixer é editável e salvo pelo Editor, e os testes de sinal passam no CI.

---

## Fase 13 — Física de produção

**Objetivo:** física estável e completa para personagem, cidade, veículos e combate.

- **Jolt:** CCD, substepping, solver settings, interpolação de render e adaptador de Job System
- **Corpos e colisores:** box, sphere, capsule, mesh, heightfield, compound, estático/dinâmico/cinemático
- **Materiais:** fricção, restituição, massa, inércia, damping, gravidade e layers
- **Joints e ragdoll:** point, hinge, cone, slider, fixed, six-DOF, ragdoll e blend com animação
- **Character controller:** rampas, escadas, plataformas móveis, crouch, push e colisão contínua
- **Veículos (fundação):** suspensão, pneus, motor, câmbio, direção, freio, drift e dano; refinamento na Fase 23
- **Queries:** raycast, shapecast, overlap e filtros
- **Debug e performance:** wireframe, contatos, forças, tempos por passo e benchmarks
- **Qualidade obrigatória:** sem tunneling em alta velocidade, sem jitter de repouso, sem ragdoll explodindo, sem plataforma atravessando o chão

**Pronto quando:** personagem sobe rampa/escada e anda em plataforma móvel sem tremer, veículo protótipo acelera/freia/curva sem instabilidade, ragdoll reage sem explodir e os testes de estresse passam no CI.

---

## Fase 14 — Scripting em C# (Roslyn)

**Objetivo:** abrir a engine para lógica de gameplay fora do C++, com API genérica e sem custo no hot path.

- Hosting do .NET/CoreCLR (nethost/hostfxr)
- Pipeline de compilação via Roslyn e hot reload com `AssemblyLoadContext` colecionável
- **Fronteira C++/C# sem marshalling no hot path:** Blittable Types, ponteiro direto para o ECS e teste de layout no CI
- **Referências de Entity via handle com geração**, sem ponteiro cru
- **API C# livre de alocação no hot path** (structs/tipos value)
- API inicial expondo renderer, física, áudio, animação, input, cena e assets
- **Inspector:** componente Script com campos públicos editáveis (int, float, bool, referência de Entity, asset)
- Testes de compilação, hot reload, GC e chamadas por frame

**Pronto quando:** um script C# compila em runtime, altera a cena sem reiniciar a engine, aparece editável no Inspector e o hot reload não vaza memória.

---

## Fase 15 — Sistema de Prefabs

**Objetivo:** criar moldes reutilizáveis com propagação de mudanças, overrides e variants — essencial para carros, pedestres, props e missões.

- Prefab como asset próprio (`.kzprefab`), serializado pela mesma capacidade genérica da Fase 3
- Instâncias referenciam o Prefab; alterar o original propaga para todas
- Overrides por instância que sobrevivem a atualizações do Prefab
- Apply/Revert e Prefab Variants
- Modo de edição isolado
- Instanciação via C# e via Scripting Visual
- Indicação visual de instância na Hierarquia

**Pronto quando:** um Prefab de carro ou inimigo é criado, instanciado várias vezes, uma instância tem override, e alterar o original propaga sem apagar o override.

---

## Fase 16 — Scripting Visual (nós)

**Objetivo:** o diferencial técnico da Kizuri: lógica visual que gera C# real, não interpretação nó a nó.

- Editor de nós com pinos de execução e dados
- Geração de C# real e legível, compilada por Roslyn
- Nós assíncronos (Delay, loops com espera) sem `Thread.Sleep`
- Compilação incremental em background
- Erro de compilação no Console e inline no nó
- Debug com valores em tempo real e breakpoints
- Menu de contexto e Inspector para anexar grafo a uma entidade

**Pronto quando:** um grafo `OnBeginPlay → PlayAnimation` funciona no Play Mode, um erro proposital aparece no Console e no nó, e o grafo é anexável como qualquer script.

---

## Fase 17 — Câmera e Input

**Objetivo:** câmeras e input prontos para personagem a pé, veículo e cinemática.

- Câmera terceira pessoa, primeira pessoa, veículo, livre e cinemática
- Colisão por raycast/shapecast em C++, configurável
- Lock-on opcional como camada de gameplay
- Shake, blends, prioridade, post-process por câmera e sequências
- Input com rebind, perfis, múltiplos dispositivos e mapeamento contextual (a pé/veículo/UI)
- **Inspector:** Camera Controller e Input Actions

**Pronto quando:** a câmera segue o personagem sem atravessar parede, a câmera de veículo funciona em velocidade, o jogador remapeia teclas e o controller é configurável pelo Inspector.

---

## Fase 18 — IA, Navegação e Multidões

**Objetivo:** agentes que navegam e se comportam bem em cidade e em combate.

- NavMesh com Recast & Detour, geração por tiles e recálculo local
- Pathfinding hierárquico e assíncrono
- Behavior trees/utility AI, percepção, memória e squad
- Group avoidance e formação
- Multidões com LOD de IA, atualização por frequência e pooling
- **Inspector:** NavAgent e Behavior; debug de NavMesh, caminho, percepção e estado

**Pronto quando:** um agente persegue o jogador desviando de obstáculos e outros agentes, uma multidão de teste roda no orçamento e o NavMesh é visualizável no Viewport.

---

## Fase 19 — Save System (do jogo)

**Objetivo:** persistência de progresso, separada do "Salvar Cena" do Editor.

- Reaproveita a serialização genérica da Fase 3
- Save slots, autosave, versionamento e migração de formato
- **Inspector:** componente Saveable
- Testes de ida e volta, corrupção e compatibilidade

**Pronto quando:** o estado de uma cena é salvo e recarregado, uma entidade é marcável como salvável e um save antigo migra sem quebrar.

---

## Fase 20 — VFX

**Objetivo:** partículas e efeitos de alta escala com custo controlado.

- Partículas GPU-based, emissores, curvas, colisão, trails, decals e sorting
- LOD de efeitos e orçamento por cena
- Editor de VFX com preview e curvas
- **Inspector:** Particle System

**Pronto quando:** milhares de partículas rodam sem impacto perceptível, um efeito é montável do zero no Editor e o custo é medido.

---

## Fase 21 — UI de jogo (HUD/menus)

**Objetivo:** ferramenta para o dev construir a interface do jogo, separada do ImGui do editor.

- Framework retained-mode próprio
- Layout, âncoras, temas, fontes e escalabilidade por DPI
- Data binding simples (vida, velocidade, munição, minimapa)
- Navegação por gamepad/teclado e acessibilidade básica
- Editor de UI com preview

**Pronto quando:** um HUD (vida + velocidade + menu de pausa) é montado visualmente sem código extra e funciona em diferentes resoluções/DPIs.

---

## Fase 22 — Streaming e mundo aberto

**Objetivo:** cidade contínua sem hitch, sem popping e sem vazamento.

- Divisão em células, carregamento/descarregamento assíncrono e orçamento por frame
- HLOD, LOD, oclusão, meshlets/GPU culling, instancing e static batching em escala
- Texture streaming por distância/célula, orçamento de VRAM, priorização e cancelamento
- Áudio e emitters carregados/descarregados com a célula
- Origin Rebasing e precisão em coordenadas grandes
- Editor de mundo/streaming, preview de célula e debug de memória
- Testes de costura, popping, hitch e vazamento

**Pronto quando:** um quarteirão carrega e descarrega sem hitch perceptível, sem costura visível, sem vazamento e dentro do orçamento de memória/VRAM.

---

## Fase 23 — Veículos

**Objetivo:** carros jogáveis de produção, integrados à cidade.

- Física de suspensão, pneus, motor, câmbio, direção, freio e drift
- Câmera de veículo, som, dano, combustível e entrada/saída
- Integração com trânsito, IA e streaming
- Editor de veículo com tuning e preview
- Testes de aceleração, frenagem, curva, impacto e estabilidade

**Pronto quando:** o jogador dirige pela cidade, colide, derrapa, ouve o motor e a câmera responde sem instabilidade.

---

## Fase 24 — Trânsito e multidões

**Objetivo:** cidade viva, com carros e pedestres em escala.

- Faixas, cruzamentos, semáforos, regras e congestionamento
- Spawning/despawning por célula e densidade configurável
- Pedestres com navegação, animação, LOD e comportamento
- Multidão com streaming, pooling e orçamento de CPU
- Debug de trânsito, rotas, densidade e agentes
- Testes de estabilidade e performance

**Pronto quando:** um quarteirão tem trânsito e pedestres coerentes, sem travamento, dentro do orçamento e com LOD funcionando.

---

## Fase 25 — Ciclo de dia/noite e clima

**Objetivo:** atmosfera dinâmica e consistente com iluminação, áudio e gameplay.

- Sol/lua, céu procedural/HDR, estrelas e fase da lua
- Hora do dia com keyframes, transições e presets por região
- Fog, nuvens, chuva, neve, vento e superfícies molhadas
- Integração com áudio ambiente e iluminação
- Editor de tempo/clima com preview
- Testes de transição e performance

**Pronto quando:** o ciclo completa 24h sem popping, o clima altera iluminação/áudio/superfícies de forma coerente e o editor permite criar presets.

---

## Fase 26 — Ferramentas de level design urbano

**Objetivo:** construir cidade no Editor, não no código.

- Terreno por heightmap e sculpting manual
- Ruas, calçadas, quarteirões, prédios, props e vegetação
- Snapping, brushes, pintura e colocação com Prefabs
- Integração com streaming/células e validação
- Undo/Redo incremental para terreno, pintura e colocação
- Ferramentas de luz, áudio e trânsito no nível

**Pronto quando:** um quarteirão completo é construído do zero no Editor, com undo/redo, streaming e boa performance.

---

## Fase 27 — Rede (opcional)

**Objetivo:** suporte a co-op quando o projeto exigir, sem contaminar o núcleo single-player.

- GameNetworkingSockets, P2P e host migration
- Client-side prediction com reconciliação/rollback
- Sincronização de posição/estado e prioridade
- **Inspector:** NetworkIdentity
- Testes de latência, perda de pacotes e migração

**Pronto quando:** dois clientes se conectam, veem um ao outro, a sessão sobrevive à saída do host e o estado converge.

---

## Fase 28 — Editor de produção e profiler

**Objetivo:** ferramentas transversais de nível profissional.

- Profiler visual (CPU/GPU/memória/frame time por sistema)
- Render graph debugger
- Validação contextual em todos os componentes
- Atalhos customizáveis, Menu Bar completo e debuggers unificados
- Asset dependency, build settings e diagnóstico de projeto
- Acessibilidade e escalabilidade de UI/DPI

**Pronto quando:** todos os sistemas são inspecionáveis no Editor e um problema de performance/asset pode ser diagnosticado sem abrir o código.

---

## Fase 29 — Documentação integrada

**Objetivo:** documentação gerada junto com o código e navegável de dentro do Editor.

- Geração a partir de comentários XML do C#
- Painel dockable com busca e `F1` contextual
- Site estático com o mesmo conteúdo
- Guias de sistemas, exemplos e limitações

**Pronto quando:** `F1` sobre um nó/função abre a página correta e a documentação cobre todos os sistemas expostos.

---

## Fase 30 — Launcher e instalador

**Objetivo:** distribuição real da engine.

- Launcher com projetos recentes, criação de projeto e `.kzproj`
- Instalador completo (Editor + Launcher + runtime .NET)
- Layout do editor em `%APPDATA%/Kizuri/`
- Versionamento engine/projeto com aviso e backup antes de migrar

**Pronto quando:** a engine instala numa máquina limpa, cria projeto, abre o Editor e um projeto antigo gera o aviso esperado.

---

## Fase 31 — Build, cook e distribuição

**Objetivo:** gerar o jogo final sem compilação C++ na máquina do dev.

- Executável base com núcleo linkado
- Cook de assets, pak, compressão e shader bytecode
- Build Debug e Release
- Análise de DLLs opcionais referenciadas
- Build 1-clique, reprodutível e assinado
- Testes de build limpo e integridade do pacote

**Pronto quando:** dois projetos (com/sem rede) geram builds diferentes, Debug/Release funcionam e o pacote roda numa máquina limpa.

---

## Fase 32 — Vertical slice urbana

**Objetivo:** provar a engine com conteúdo real, não com cena de teste.

- Quarteirão jogável com ruas, prédios, carro, pedestres, trânsito e missão simples
- Ciclo de dia/noite, clima, áudio, UI, save e streaming
- Benchmark no hardware mínimo e recomendado
- Correção de bugs encontrados e ajuste dos orçamentos

**Pronto quando:** a slice é jogável do início ao fim, sem bug P0/P1, dentro da meta de frame time no hardware mínimo e com números reais registrados.

---

## Fase 33 — Endurecimento de produção

**Objetivo:** transformar a vertical slice em base confiável para o jogo completo.

- Crash reporting, telemetria e diagnóstico
- Matriz de QA (hardware, resolução, DPI, idioma, save)
- Performance, memória, carregamento e estabilidade
- Acessibilidade, localização e polish de UI
- Checklist de release, issues conhecidas e documentação final
- Projeto de exemplo e validação de instalador/build

**Pronto quando:** a engine e a slice passam na matriz de QA, não há vazamento/crash conhecido, os orçamentos são cumpridos e existe um release candidate documentado.

---

## Itens em aberto (ainda não decididos)

As decisões abaixo continuam abertas e serão fechadas antes da fase correspondente:

### Streaming de mundo (em escopo, estratégia a definir)
O streaming agora é requisito do primeiro jogo urbano. A arquitetura já tem Origin Rebasing (Fase 1), mas o tamanho de célula, a estratégia de HLOD, o orçamento de VRAM e a política de despejo serão definidos na Fase 22 com a cena de referência.

### ECS como fonte única (Fase 9.5)
A migração 100% ECS continua opcional e não aprovada. A arquitetura híbrida (Scene para autoria/serialização, ECS para runtime) segue como base. A decisão será reavaliada antes de Prefabs e do mundo aberto.

### Rede
Co-op é opcional. O primeiro jogo é single-player; a Fase 27 só entra se o projeto exigir. A arquitetura não pode impedir rede, mas não será contaminada por ela.

### Estilo visual
A base é PBR realista, com cel shading como modo de material/câmera. A proporção entre realismo e estilo no primeiro jogo ainda será definida com uma cena de referência.

### Escopo do primeiro jogo urbano
Tamanho da cidade, quantidade de interiores, densidade de trânsito/pedestres, multiplayer e plataformas de lançamento ainda não estão fechados. A vertical slice da Fase 32 define o orçamento real.

### Física de veículos
A decisão entre modelo de raycast, sweep ou corpos rígidos compostos no Jolt será tomada na Fase 13 com testes de estabilidade, e refinada na Fase 23.

### Hardware e presets
A tabela do TDD define piso de compatibilidade, mínimo de produção e recomendado. Os presets Baixo/Médio/Alto/Ultra serão calibrados com a vertical slice e não por estimativa.

### Escopo da API C# e do Scripting Visual
A API inicial cobre os sistemas existentes na Fase 14; a superfície definitiva cresce junto com os sistemas seguintes e é documentada na Fase 29.

---

## Ordem de dependência resumida

```
Fase 0  → Repositório
Fase 1  → Núcleo (alocadores, job system, RHI, DX11, Origin Rebasing)
Fase 2  → Renderer básico
Fase 3  → Editor Shell completo (painéis, serialização/Salvar Cena, autosave,
           Undo/Redo genérico, seleção múltipla, notificações, menus de contexto)
Fase 4  → Pipeline de Assets + Asset Browser + dependência entre assets
Fase 5  → Iluminação/sombras avançadas + céu procedural + Inspector de Light
Fase 6  → Física (Jolt) + Inspector de Rigidbody/Ragdoll
Fase 7  → Áudio + Mixer completo (buses, ducking, reverb) + Inspector de AudioSource
Fase 8  → Runtime de Jogo (GameLoop, GameWorld, GameApp, KizuriRuntime)
Fase 9  → ECS/Gameplay Framework + "Add Component" genérico + parenting + Play-in-Editor
Fase 9.5 (opcional, proposta) → ECS como fonte única; só se aprovada antes de Prefabs/mundo aberto
Fase 10 → Animação completa (skinning, state machine, camadas, IK, root motion, retargeting)
Fase 10.5 → Estabilização crítica (Material, luz, viewport, TAA, cenas de referência)
Fase 11 → Renderização de produção (cor, sombras, iluminação, pós, materiais, cel shading)
Fase 12 → Áudio de produção (mixer, DSP, spatialização, oclusão, streaming)
Fase 13 → Física de produção (CCD, character controller, veículos fundação, ragdoll)
Fase 14 → Scripting C#/Roslyn + Inspector de Script
Fase 15 → Sistema de Prefabs (overrides, variants, apply/revert)
Fase 16 → Scripting Visual (nós) + exibição de erro inline
Fase 17 → Câmera/Input (personagem, veículo, cinemática) + Inspector
Fase 18 → IA/Navegação/Multidões (NavMesh, behavior, avoidance)
Fase 19 → Save System do jogo (reaproveita serialização da Fase 3)
Fase 20 → VFX + Editor de partículas
Fase 21 → UI de jogo (HUD/menus)
Fase 22 → Streaming e mundo aberto (células, HLOD, texturas, áudio, memória)
Fase 23 → Veículos (física, câmera, som, dano, trânsito)
Fase 24 → Trânsito e multidões (faixas, semáforos, pedestres, LOD)
Fase 25 → Ciclo de dia/noite e clima
Fase 26 → Ferramentas de level design urbano (ruas, quarteirões, prédios)
Fase 27 → Rede P2P opcional
Fase 28 → Editor de produção e profiler
Fase 29 → Documentação integrada
Fase 30 → Launcher/Instalador + versionamento engine/projeto
Fase 31 → Build, cook e distribuição
Fase 32 → Vertical slice urbana
Fase 33 → Endurecimento de produção
```
