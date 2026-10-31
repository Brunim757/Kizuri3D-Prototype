# Kizuri Engine — Documento de Design Técnico (TDD)

**Versão:** 1.0 (direção de produção)
**Data:** Outubro de 2026
**Autor:** Kizuri Studio

---

## Regras

1. Sem comentários em código
2. Build via GitHub — comando `gh` já configurado e vinculado à conta
3. Não compilar nem rodar nada localmente — este ambiente não é para isso
4. Repositório: `KizuriEngine`, público no GitHub (mesmo sendo a engine proprietária — licença de uso restrita, mas código visível)
5. Testes: tudo que **não depende de interface** deve ser testado no próprio GitHub (CI); o que **depende de interface** (Editor, Viewport, janelas) fica pendente até o autor testar localmente no PC e validar visualmente
6. Sempre **verificar os arquivos e o fluxo do código** antes de considerar uma tarefa concluída — revisar se a lógica está correta ponta a ponta, não só se compila. Um erro de uma única linha pode fazer algo parecer implementado mas não funcionar de fato (ex.: código que deveria renderizar um cubo, compila sem erro, mas por uma linha errada o cubo não aparece na tela)
7. **Proibido placeholder, stub ou implementação simplificada "pra depois".** Toda feature implementada precisa funcionar de ponta a ponta antes de ser considerada parte do sistema — isso inclui a exposição de componentes no Editor: se um sistema (Física, Animação, Scripting, etc.) ganha um componente novo no ECS, o desenho correspondente no Inspector é parte obrigatória da mesma entrega, não uma tarefa adiada para uma fase de "polimento" futura
8. **O CI deve entregar artefato de build desde a Fase 0**, não só reportar verde/vermelho. Todo pipeline do GitHub Actions publica o executável/binários compilados como artifact baixável (ex.: `actions/upload-artifact`), para que o autor consiga pegar e testar localmente (regra 5) sem precisar compilar do zero na própria máquina
9. **DEVE REVISAR TODOS OS ARQUIVOS QUE FORAM ADICIONADOS, MEXIDOS E VER SE A IMPLEMNTACAO ESTA CORRETA PARA NAO TER PROBELMAS
10. DEVE PESQUISAR A API DAS LIBS QUE FOR MEXER PARA ADICIONAR PERFEITAMNTE
11. DEVE ESTUDAR A PROPRIA ENGINE PARA ADICIONAR TUDO COM PERFEIÇÃO 
12. SEMPRE LER ESSE ARQUIVO E O ROADMAP ANTES DE COMECAR UMA FASE
13. SO INICIAR UMA FASE QUANDO A ANTERIOR ESTIVER 100%
14. E PARA FAZER OQUE ESTA DESCRITO AQUI NO TDD E NO ROADMAP NAO ALGO SIMPLES EQUIVALENTE FAZER ALGO SIMPLES ESTA ESTRITAMENTE PROIBIDO EM UMA GAME ENGINE VC DEVE FAZER TUDO O MAIS AVANCADO E COMPLETO POSSIVEL NAO ALGO SIMPLES QUE FUCNIONA EU QUERO EXATAMNTE OQUE O TDD E O ROADMAP FALAM SEM SIMPLIFICAR SO PARA ENTREGAR MAIS RAPIDO EU NAO LIGO PARA O TEMPO QUE VAI DEMORAR EU NAO LIGO PARA O TAMANHO QUE VAI FICAR MAS TEM QUE FEITO DE VERDADE SIMPLIFCAR NAO É FAZER BEM FEITO
15. **Padrão de produção é obrigatório.** Nenhum sistema é considerado pronto por "funcionar no caso feliz". Ele precisa ser correto, testado, perfilado, documentado, editável no Editor e coberto por serialização, Undo/Redo e Inspector quando aplicável.
16. **Nenhuma feature entra pela metade.** Se um sistema novo é adicionado, a entrega inclui runtime, Editor, serialização, testes, debug visual, orçamento de performance e documentação na mesma fase. Placeholder, stub, "depois eu vejo" e implementação simplificada continuam proibidos.
17. **Qualidade visual é medida, não opinada.** Cenas de referência, capturas de regressão visual, comparação de estabilidade temporal (flicker, TAA, sombras) e orçamentos de frame time fazem parte do critério de pronto de qualquer trabalho gráfico.
18. **Não existe "feature solta".** Um efeito, componente ou sistema só conta quando está integrado ao pipeline, ao Editor, ao runtime e aos testes, e quando se comporta de forma consistente em todas as resoluções, DPIs e hardwares suportados.
19. **O núcleo é genérico.** A engine não pertence a um gênero. Souls-like, jogo urbano, cel shading e qualquer outro estilo são camadas construídas sobre o mesmo núcleo. O primeiro jogo é urbano, mas nenhuma decisão de arquitetura pode fechar a porta para os outros.
20. **Referência de ambição: RAGE.** GTA V é DX11 e continua sendo referência visual e técnica porque a engine por trás é madura. A Kizuri não copia a RAGE; persegue o mesmo nível de completude, ferramentas, consistência e estabilidade.
21. **Devlog em primeira pessoa, sem saudação artificial.** A estrutura antiga de "o que foi feito / o que vem" é mantida, mas o texto começa direto em "corrigi", "removi", "melhorei", "arrumei". Sem "e aí, pessoal", sem nomes de arquivos internos e sem lista robotizada de correções linha a linha.
---

## 1. Visão geral

A **Kizuri Engine** é uma game engine 3D completa, desenvolvida do zero em C++ com scripting em C#, para Windows e DirectX 11. O núcleo é **genérico e independente de gênero**: a mesma engine deve conseguir construir um jogo urbano de mundo aberto, um action RPG souls-like, um jogo estilizado em cel shading ou qualquer outro projeto 3D.

O primeiro jogo feito na engine será **urbano**, no espírito de GTA/Just Cause: cidade contínua, ruas, prédios, carros, pedestres, trânsito, missões, ciclo de dia/noite e clima. Souls-like continua suportado como **camada de gameplay e animação**, mas deixa de definir a arquitetura. Cel shading também é um modo de renderização de primeira classe, não um experimento à parte.

A referência de ambição é a **RAGE**. GTA V é DX11 e continua sendo um jogo tecnicamente impressionante porque a engine por trás é madura: sistemas completos, ferramentas de produção, consistência visual, estabilidade e desempenho. A Kizuri não copia a RAGE; persegue o mesmo tipo de completude. A API gráfica não é o limite — a qualidade da implementação é.

### 1.1 Motivação

Este é um projeto pessoal, motivado por engenharia de motores de jogo e por construir uma base capaz de sustentar jogos grandes de verdade. O primeiro objetivo concreto é um jogo urbano com cidade, carros e pessoas. O segundo é provar que a mesma base consegue entregar um souls-like completo e jogos estilizados. Não há marketplace, não há plugin de terceiros e não há intenção de competir em escopo com Unity/Unreal; há intenção de fazer os sistemas que existem com qualidade de produção.

### 1.2 Filosofia central

- **Qualidade de produção acima de quantidade de features.** Um sistema só conta quando é correto, estável, testado, perfilado, documentado e editável no Editor.
- **Performance é requisito não-negociável.** A engine deve sustentar cidade, trânsito, personagens e efeitos sem stutter, sem travamento de interface e sem quedas perceptíveis de frame.
- **Consistência visual e temporal.** Cor, exposição, sombras, TAA, animação e áudio precisam ser estáveis. Tremulação, flicker, rachaduras de luz, popping e ghosting são bugs de produção, não "detalhes".
- **Padrão arquitetural repetido:** a engine fornece sistemas robustos e configuráveis; o jogo decide comportamento e tuning. A engine nunca impõe design de gameplay.
- **Núcleo genérico, camadas opcionais.** O que é específico de souls-like, de cidade ou de cel shading vive em camadas e modos, não no coração do runtime.
- **Ferramenta e teste fazem parte da feature.** Inspector, serialização, Undo/Redo, debug visual, profiler e testes no CI não são polimento posterior.
- **Escalabilidade real.** A GT 610 é piso de compatibilidade e detector de custo patológico; a fidelidade visual é medida em hardware moderno. Nenhuma degradação automática esconde problema.
- **Sem placeholder, stub ou "depois eu arrumo".** O que entra precisa funcionar de ponta a ponta.

### 1.3 Requisitos do primeiro jogo (urbano)

O primeiro jogo impõe requisitos técnicos que a engine precisa suportar de verdade:

- Cidade contínua com streaming de células, LOD, oclusão, HLOD e carregamento assíncrono sem hitch
- Ruas, calçadas, quarteirões, prédios, interiores e props com ferramentas de level design
- Carros com física, suspensão, câmera, som, danos e integração com trânsito
- Pedestres e multidões com animação, navegação, LOD e streaming
- Trânsito com faixas, cruzamentos, semáforos, spawning e despawning
- Ciclo de dia/noite, clima, neblina, iluminação e áudio ambiente
- Missões, save/load, UI de jogo, input e localização
- Áudio com mixer completo, buses, reverb zones, oclusão e streaming
- Renderização estável em escala de cidade, com sombras, reflexos, TAA e pós-processamento

### 1.4 Suporte a outros gêneros

A mesma base deve suportar:

- **Souls-like:** combate comprometido, stamina, lock-on, boss fights, level design interconectado, checkpoint e progressão. Nada disso vive no núcleo; é camada de gameplay.
- **Cel shading:** modo de material com rampa quantizada de luz direta e ambiente, sombras pintadas, contorno por normal/depth, especular estilizado e rim; convive com PBR na mesma cena e não substitui o caminho padrão.
- **Outros estilos 3D:** a arquitetura não pode impedir FPS, RPG, survival, plataforma ou projetos experimentais.

---

## 2. Arquitetura geral

### 2.1 Estrutura de build

- **1 executável final** por jogo, gerado pelo sistema de Build do editor
- **Núcleo sempre presente**: sistemas que todo jogo feito na Kizuri sempre usa (`KizuriRender`, `KizuriPhysics`, `KizuriAnimation`, `KizuriAudio`, `KizuriScripting`, `KizuriCore` — ECS, job system, alocadores) são compilados como `.lib` estáticas **internamente, pelo Kizuri Studio**, e linkadas dentro de um único executável base pré-compilado: `KizuriRuntime.exe`. As `.lib` em si **nunca são distribuídas** — são um artefato interno do processo de build da própria engine, sem utilidade para quem não tem um linker C++ (e o SDK não expõe compilação C++ a ninguém, por decisão já registrada na seção sobre o SDK)
- **Sistemas opcionais, como `.dll` pré-compiladas com carregamento tardio**: sistemas que nem todo jogo precisa — hoje, o caso claro é `KizuriNetworking.dll`; outros módulos opcionais que surgirem no futuro seguem o mesmo padrão

**Correção arquitetural (v0.4):** versões anteriores deste documento cogitaram distribuir `.lib` diretamente ou gerar múltiplos executáveis por combinação de módulos — ambas descartadas. A primeira não faz sentido porque o estúdio que usa a engine nunca tem um toolchain de compilação C++ para consumir uma `.lib`; a segunda gera explosão combinatória conforme módulos opcionais aumentam. A solução final combina o melhor dos dois problemas resolvidos: **um único executável base pré-compilado para o núcleo (sempre igual) + DLLs individuais só para o que é genuinamente opcional**.

**O que o SDK distribui de fato:**
- `KizuriRuntime.exe` — o executável base com o núcleo já linkado, pronto para receber assets e C# de qualquer projeto
- As `.dll` de sistemas opcionais, pré-compiladas
- Editor, Launcher

**O que o Build faz, dentro do Editor, sem nenhuma compilação C++:**
1. Copia o `KizuriRuntime.exe` e renomeia para o nome do jogo (ex.: `MeuJogo.exe`)
2. Empacota os assets e o C# compilado do projeto como dados carregados em runtime pelo executável base (não como algo linkado)
3. Analisa quais sistemas opcionais o projeto referencia em C# e copia as `.dll` correspondentes para a mesma pasta

Exemplo prático:
```
SDK distribuído (instalador):
├── Editor.exe / Launcher.exe
└── Runtime/
    ├── KizuriRuntime.exe       (núcleo já linkado, executável base pronto)
    └── KizuriNetworking.dll    (pré-compilada, copiada só se usada)

Build de um projeto sem rede:
MeuJogo/Build/
├── MeuJogo.exe                 (cópia renomeada do KizuriRuntime.exe)
└── Data/                       (assets + C# compilado do projeto)

Build de um projeto com rede:
MeuJogo/Build/
├── MeuJogo.exe
├── Data/
└── KizuriNetworking.dll        (copiada automaticamente pelo Build)
```

### 2.2 Camadas da engine

```
┌─────────────────────────────────────────┐
│   Gameplay (C# + Scripting Visual/Nós)   │  ← Dev do jogo trabalha aqui
├─────────────────────────────────────────┤
│   API C# (assemblies expostas)           │  ← Contrato entre C# e C++
├─────────────────────────────────────────┤
│   Core Engine (C++, ECS híbrido)         │  ← Renderer, física, áudio,
│   Job System + Alocadores próprios       │    animação, scripting host
└─────────────────────────────────────────┘
```

- **Núcleo em C++**: sistemas de baixo nível, performance crítica (renderer, física, job system, alocadores)
- **ECS híbrido**: dados organizados como Entity Component System (cache-friendly, paralelizável via job system) por baixo, mas exposto em C# através de uma API que se parece com componentes tradicionais — o dev não precisa entender ECS para usar a engine
- **Scripting**: toda lógica de gameplay é escrita em C# ou via scripting visual por nós (que compila para C# real via Roslyn, não é interpretado em runtime)

### 2.3 Plataforma e API gráfica

- **Plataforma alvo: Windows apenas** (sem necessidade de portabilidade)
- **API gráfica: DirectX 11**
- **RHI (Render Hardware Interface)**: camada fina de abstração entre o renderer e a API gráfica (`CreateBuffer()`, `CreateTexture()`, `Draw()`, etc.). O renderer nunca chama DX11 diretamente — chama a RHI, que hoje traduz para DX11 e no futuro pode ganhar um backend DX12/Vulkan sem exigir reescrita do renderer. **Decisão arquitetural obrigatória desde o início**, dado o alto custo de adicionar isso retroativamente.

### 2.4 SDK e distribuição

- SDK **100% baseado em C#/scripting visual** — nenhuma exposição de C++ nativo a terceiros
- Distribuição via **instalador completo** (Editor + Launcher + runtime .NET necessário), registra associação de arquivo `.kzproj`
- Conteúdo do SDK: Editor, Launcher, assemblies C# (contrato de API), documentação (gerada dos comentários XML do C#), exemplos

### 2.5 Padrão de entrega e portões de qualidade

Nenhuma fase ou correção é considerada concluída apenas por compilar ou por "funcionar na cena feliz". Todo trabalho passa pelos portões abaixo:

1. **Ponta a ponta:** runtime, Editor, serialização, Undo/Redo, Inspector e persistência quando aplicável.
2. **Teste automatizado:** unidade e integração no CI; teste visual/imagem quando o resultado for gráfico; teste de performance quando houver custo relevante.
3. **Sem regressão conhecida:** nenhum bug P0/P1 aberto no sistema tocado; artefatos visuais conhecidos contam como bug.
4. **Orçamento de performance:** frame time, memória, VRAM e tempo de carregamento medidos contra a cena de referência.
5. **Ferramenta e debug:** o sistema precisa ser inspecionável e configurável no Editor, com debug visual quando fizer sentido.
6. **Documentação:** API, comportamento, limitações e decisão de design registrados.
7. **Revisão de arquivos:** todos os arquivos tocados são relidos e verificados antes do commit.
8. **Commit único e CI verde:** uma correção verificada por commit; a próxima mudança só começa depois do CI concluir.

---

## 3. Stack de bibliotecas de terceiros

Uma biblioteca definitiva por categoria — sem alternativas em aberto — todas com licença permissiva compatível com engine proprietária fechada.

| Categoria | Biblioteca | Licença |
|---|---|---|
| Física | Jolt Physics | MIT |
| Navegação | Recast & Detour | zlib |
| Áudio | miniaudio | MIT / Public Domain |
| Scripting C# | Roslyn (Microsoft.CodeAnalysis.CSharp) | MIT |
| Host .NET/CoreCLR | nethost / hostfxr | MIT |
| Matemática | DirectXMath | MIT |
| Import de modelos 3D | cgltf | MIT |
| Import de texturas | stb_image | Public Domain |
| Compressão/mipmap de textura | DirectXTex | MIT |
| Job System (multithread) | enkiTS | zlib |
| Rede P2P | GameNetworkingSockets | BSD-3-Clause |
| Editor/UI de ferramentas | Dear ImGui | MIT |
| Gizmo de transformação | ImGuizmo | MIT |
| Ícones do editor | Fork Awesome + IconFontCppHeaders | MIT / OFL |
| Fontes/texto | FreeType | FTL/BSD-like |
| Serialização (save/assets) | FlatBuffers | Apache 2.0 |
| Compressão de assets | zstd | BSD-3-Clause |
| Janela/Input | Win32 API nativo | (sistema operacional) |

A engine usa bibliotecas maduras onde elas são a melhor escolha de engenharia — física, navegação, decodificação, compressão — e concentra o desenvolvimento próprio no que é diferencial: renderer, pipeline de assets, animação, áudio mixer, ferramentas, runtime e integração. "Feito do zero" não significa reimplementar o que uma biblioteca consolidada faz melhor.

---

## 4. Sistemas centrais (Core Engine)

### 4.1 Renderização

A renderização é tratada como sistema de produção, não como coleção de efeitos. O critério é imagem estável, cor consistente, sombras sem artefatos e custo previsível.

- **Pipeline:** Deferred Rendering para opacos + caminho Forward+ para transparentes, partículas e materiais que precisam de iluminação por pixel; render graph explícito com dependências e orçamento por passe
- **Cor:** workflow linear, sRGB correto em texturas e alvos, exposição manual e automática, tonemap configurável (ACES e alternativas), LUT, contraste, saturação, balanço de branco e consistência entre Editor e runtime. A exposição manual multiplica a auto-exposição em vez de ser descartada, e volumes de pós tratam temperatura/tint como deslocamento relativo sobre a base da câmera
- **Sombras:** Cascaded Shadow Maps com cascatas estáveis (sem shimmer), bias automático, normal offset, PCF/PCSS, contato, sombras de point/spot, atlas, cache e resolução dinâmica com critério. Os casters usam o mesmo alpha cutout, dither de LOD e deslocamento por height map do G-buffer, com tesselação própria no depth pass e culling coerente com materiais double-sided, para que silhueta e profundidade coincidam com a cena visível.
- **Iluminação:** direcional, point, spot, area/rect, IBL, reflection probes, DDGI, light layers, cookies, unidades físicas, temperatura de cor e luzes em quantidade sem queda de frame
- **Pós-processamento:** SSAO/GTAO, SSR, SSGI, TAA com velocity buffer, FXAA, bloom, DOF, motion blur, god rays, film grain, vinheta, heat haze e fog volumétrico — todos com ordem, qualidade e custo documentados
- **Contrato do pós-processamento:** o caminho de backbuffer e o caminho de textura executam o mesmo pipeline offscreen, com iluminação HDR, auto-exposição, tonemap, grade, bloom, antialiasing, FSR e blit final. O Editor exibe o alvo final e o runtime o apresenta no backbuffer sem reencodar cor. Volumes de pós definem exposição, tonemap, contraste, saturação e bloom de forma absoluta, e temperatura/tint como deslocamento sobre a base, com clamps iguais aos do renderer. Cenas antigas sem esses campos leem zero e permanecem visualmente idênticas
- **Contrato do TAA:** pixels estáticos reprojetam pela posição reconstruída do depth buffer usando as matrizes sem jitter (`PrevVPU`/`CurrVPU`); objetos com transform ou ossos alterados entram num velocity buffer separado, desenhado contra o `gDepth` com `LessEqual` e consumido no slot 3 do resolve. O velocity pass reproduz alpha cutout, VT, dither de LOD e double-sided para não gravar movimento em pixels que o G-buffer descartou. Superfícies tessellated usam hull/domain próprios do velocity, com os mesmos fatores de tesselação e o mesmo deslocamento por height map do G-buffer, e calculam a posição anterior no espaço de mundo anterior para que a mudança de profundidade causada pelo deslocamento não gere ghosting. Alterações de tesselação invalidam o histórico temporal, porque silhueta e deslocamento mudam sem transformação de objeto. O histórico é rejeitado por profundidade e por ausência de pose anterior, e o primeiro frame de um objeto resolve no frame atual. O passe só roda com TAA ativo e o alvo é limpo uma vez por frame; objeto parado não entra no buffer.
- **Materiais:** PBR completo e correto (energia, Fresnel, normal maps, ORM, alpha modes, double-sided), clearcoat, sheen, transmission, volume, anisotropia, iridescência, dispersão, layered materials, decals, detail maps, POM, tessellation e virtual textures
- **Geometria:** meshlets, GPU culling, oclusão, LOD, instancing, static batching e streaming; malhas skinned ficam fora de meshlets, LOD bake, instancing, GPU culling e static batching
- **Cel shading:** modo de material com rampa quantizada de luz direta e ambiente, sombras pintadas, especular estilizado, rim e contorno por normal/depth; os parâmetros vivem no material, convivem com PBR na mesma cena e não substituem o caminho padrão; presets de estilo preenchem esses parâmetros e ligam o modo
- **Debug e profiler:** debug views, visualização de G-buffer, sombras, luzes, probes, overdraw, HZB, tempos de GPU por passe, shader hot reload e captura de frame
- **Qualidade obrigatória:** sem flicker de luz, sem rachaduras de sombra, sem costura de cascata, sem ghosting de TAA, sem popping de LOD e sem diferença de cor entre Editor e jogo

### 4.2 Física — Jolt Physics

- **Backend:** Jolt Physics com adaptador de Job System para o enkiTS, passo fixo, substepping e interpolação de render
- **Corpos e colisores:** rigidbody estático/dinâmico/cinemático, box, sphere, capsule, mesh, heightfield e compound
- **Materiais e resposta:** fricção, restituição, massa, inércia, damping, gravidade e layers de colisão
- **Joints e ragdoll:** point, hinge, cone, slider, fixed, six-DOF, ragdoll configurável e blend com animação
- **Character controller:** rampas, escadas, plataformas móveis, crouch, push, colisão contínua e resposta estável
- **Veículos:** suspensão, pneus, motor, câmbio, direção, freio, drift, dano e integração com trânsito — como sistema da engine, não como script improvisado
- **Queries:** raycast, shapecast, overlap, filtros por layer e detecção de contato
- **Qualidade obrigatória:** sem tunneling em alta velocidade, sem jitter de repouso, sem ragdoll explodindo, sem plataforma atravessando o chão e com debug visual completo

### 4.3 Áudio

- **Base:** miniaudio para device, decodificação e streaming
- **Mixer:** grafo de buses hierárquico (Master → Música, SFX, Voz, Ambiente, UI), sends/returns, inserts, volume, mute, solo, snapshots e ducking com sidechain
- **DSP:** EQ, compressor, limiter, reverb, delay, filtros, distortion e medidores por bus
- **Espacialização:** atenuação por distância, panning, Doppler, oclusão, reverb zones, ambiência e emitters 3D
- **Streaming:** clipes longos com streaming, voice management, prioridade e limite de vozes
- **Editor:** Audio Mixer completo, roteamento visual, curvas, preview, salvamento como asset e Inspector de AudioSource
- **Qualidade obrigatória:** sem clique, sem estalo, sem corte abrupto, sem vazamento entre buses, com latência e custo medidos

### 4.4 Animação

- **Núcleo:** esqueleto, skinning por GPU, importação glTF, amostragem de clipes e conversão de CUBICSPLINE para linear a 60 Hz no import
- **Runtime:** state machine, blend trees, camadas, máscaras, additive, sync groups, eventos e root motion
- **IK:** pés no terreno, mãos em objetos, look-at de cabeça/tronco e ajuste de coluna
- **Retargeting:** rig humanoide padrão, mapeamento de ossos e reuso de animação entre personagens
- **Integração:** hitboxes sincronizadas no mesmo passo da animação, ragdoll blend e animação de multidão
- **Editor:** state machine, blend tree, timeline, preview, debugger de esqueleto/pesos e Inspector de Animator
- **Qualidade obrigatória:** sem popping entre clipes, sem skinning quebrado, sem hitbox atrasada e com custo previsível em muitos personagens

### 4.5 IA e Pathfinding

- **NavMesh:** Recast & Detour, com geração por tiles e recálculo apenas da região afetada
- **Pathfinding:** hierárquico e assíncrono, adequado a cidade grande e a interiores
- **Agentes:** behavior trees/utility AI, percepção, memória, group avoidance, formation e squad
- **Multidões e trânsito:** agentes em massa com LOD de IA, spawning/despawning, faixas, cruzamentos e semáforos
- **Depuração:** visualização de NavMesh, caminhos, agentes, percepção e estado de comportamento no Viewport

### 4.6 Rede

- Recurso **opcional**, fora do caminho crítico do primeiro jogo single-player
- Modelo P2P com host migration para co-op, usando GameNetworkingSockets
- Client-side prediction com reconciliação/rollback para jogos que precisarem
- A arquitetura do runtime não pode impedir rede, mas o jogo urbano inicial não depende dela

### 4.7 VFX

- Sistema de partículas **GPU-based**, com emissores, curvas, colisão, trails, decals e sorting
- Milhares de partículas sem impacto perceptível de performance
- Editor de VFX com preview no Viewport e curvas editáveis

### 4.8 Câmera

- **Câmeras built-in:** terceira pessoa, primeira pessoa, veículo, livre e cinemática
- Colisão por raycast/shapecast em C++, configurável via componente
- **Lock-on** é camada opcional de gameplay, não responsabilidade fixa do núcleo
- Suporte a shake, blends, prioridade, post-process por câmera e sequências cinemáticas

### 4.9 Sistema de Prefabs

- **Prefab** é um asset próprio (`.kzprefab`) que guarda um "molde" de entidade (ou hierarquia de entidades), serializado pela mesma capacidade de serialização genérica por reflection descrita na seção 4.10
- Instâncias colocadas numa cena referenciam o Prefab original; alterar o Prefab **propaga automaticamente** para todas as instâncias existentes
- **Overrides por instância**: cada instância pode sobrescrever propriedades específicas sem perder a ligação com o Prefab original; um override sobrevive a atualizações futuras do Prefab
- **Apply/Revert**: uma instância pode empurrar sua mudança de volta ao Prefab original (propagando a todas as outras instâncias) ou descartar o override (voltando ao valor do Prefab)
- **Prefab Variants**: um Prefab pode herdar de outro, sobrescrevendo apenas parte de suas propriedades (ex.: variações de um mesmo tipo de inimigo)
- **Modo de edição isolado**: editar um Prefab abre um contexto próprio, fora de qualquer cena, evitando alterar sem querer apenas uma instância específica
- Instanciável tanto via API C# quanto por nó de Scripting Visual — é o mecanismo central para popular uma cidade com carros, pedestres, props e missões sem duplicação manual de trabalho

### 4.10 Save System (do jogo)

- Sistema de **serialização genérica baseada em reflection** (qualquer componente/entidade pode ser serializado automaticamente) — construída cedo, já na fundação do Editor (Roadmap, Fase 3), e reaproveitada por quatro consumidores diferentes: "Salvar Cena" do Editor, snapshot de Play-in-Editor, Sistema de Prefabs, e o Save System do jogo descrito aqui
- Gerenciador de save slots (múltiplos saves, autosave), exposto ao jogo via C#
- O conceito de "checkpoint"/ponto de salvamento (o que é salvo, quando) é decisão do jogo, construída sobre a serialização fornecida pela engine
- **Não deve ser confundido com "Salvar Cena" do Editor** — são consumidores diferentes da mesma capacidade de serialização: um persiste o estado de autoria (a cena que o dev está montando), o outro persiste o progresso do jogador em runtime

### 4.11 Input

- Rebind completo de teclas/botões, com perfis por dispositivo
- Suporte a múltiplos dispositivos de controle simultâneos
- Mapeamento contextual: personagem a pé, veículo, câmera, UI e editor
- Input crítico via polling de baixo nível, fora do loop de mensagens do Win32

### 4.12 Threading e memória — requisito não-funcional crítico

- **Job System próprio** (enkiTS) para paralelizar streaming, física, animação e outras cargas pesadas
- **Alocadores customizados** (pool allocators, arena/stack allocators por sistema) — evitar `malloc`/`new` no hot path, minimizar fragmentação
- **Regra obrigatória**: nenhuma operação de I/O ou processamento pesado roda na thread principal/UI. Toda operação pesada passa pelo job system, com comunicação assíncrona de volta à interface via fila de eventos. Isso é o que garante que o Editor (e o jogo) nunca travem a interface durante carregamentos — requisito citado explicitamente como problema em projetos anteriores do autor e que a Kizuri deve resolver estruturalmente.

### 4.13 Localização

- Apenas inglês no lançamento inicial
- Boas práticas desde já: todo texto passa por um sistema de "string por chave" (mesmo com um idioma só), para não travar expansão futura

### 4.14 Modding

- Não é responsabilidade nativa da engine (decisão do jogo/estúdio que a utiliza)
- Arquitetura não deve *impedir* modding futuro, mas não é requisito ativo

### 4.15 Streaming e mundo aberto

- **Cidade contínua:** divisão em células, carregamento/descarregamento assíncrono e transição sem hitch
- **Geometria:** HLOD, LOD por distância, oclusão, meshlets/GPU culling e instancing em escala
- **Texturas e assets:** streaming por distância, orçamento de VRAM, priorização e cancelamento de requisições
- **Áudio:** zonas e emitters carregados/descarregados junto com as células
- **Precisão:** Origin Rebasing para manter coordenadas estáveis longe da origem
- **Memória:** orçamento por célula, pool de recursos, despejo previsível e telemetria
- **Qualidade obrigatória:** sem popping visível, sem travamento na thread de render, sem vazamento de memória e sem costura entre células

### 4.16 Ciclo de dia/noite e clima

- Sol/lua, céu procedural e HDR sky, fase da lua, estrelas e iluminação coerente com a hora
- Fog, névoa, nuvens, chuva, neve, vento e superfícies molhadas
- Transições suaves, presets por região e integração com áudio ambiente
- Editor de time of day com preview e keyframes

### 4.17 Cel shading

- Modo de material com rampa quantizada de luz direta e ambiente, sombra coerente com o shadow map, especular estilizado, rim e emissivo controlado
- Contornos por normal/depth em espaço de tela, com espessura em pixels e cor próprios por material, aplicados no caminho deferred (opacos) apenas em pixels de materiais cel
- Sombras pintadas: na luz direcional sombreada, a parcela direta mistura para uma cor de sombra própria do material, com força configurável; sem o modo cel ou com força zero, o resultado é idêntico ao caminho PBR
- Os parâmetros são serializados no material e por parte, editáveis no Inspector e no painel de material, com valores padrão seguros para cenas que não usam o modo
- O modo convive com PBR na mesma cena, usa a mesma tabela de materiais do deferred e é resolvido no passe de iluminação, sem passe extra e sem alterar o G-buffer
- Compatibilidade com sombras, TAA e pós-processamento: o contorno e o banding entram no resultado final antes do fog e do pós, e o histórico temporal continua sendo rejeitado pelos mesmos critérios de profundidade e movimento
- Presets de estilo (anime, comic, pintado e técnico) preenchem os parâmetros do material e ligam o modo, sem criar estado paralelo; preview dedicado fica para a camada de autoria sobre esse contrato

---

## 5. Editor

### 5.1 Aparência e layout

- **Tema dark** por padrão (ferramenta profissional, uso prolongado)
- **Menu Bar** (File, Edit, Assets, Entity, Window, Help) acima da **Toolbar** (Play, Save, Settings)
- Estrutura de painéis dockable e redimensionáveis, podendo ser destacados para outra tela:
  - **Hierarquia** (árvore de entidades da cena)
  - **Viewport 3D** com gizmos de transformação e Play-in-Editor
  - **Inspector** (componentes da entidade selecionada)
  - **Asset Browser** (modelos, texturas, áudio, scripts)
  - **Console** (logs)
  - **Editor de Terreno/Vegetação** (sculpting manual via heightmap + pintura de vegetação com instancing — sem geração procedural)
  - **Editor de Materiais**
  - **Editor de Scripting Visual** (nós)
  - **Editor de Animação** (state machine, blend tree, preview de IK)
  - **Editor de VFX/Partículas**
  - **Audio Mixer** (buses, sends, inserts, DSP, snapshots, medição)
  - **Editor de Mundo/Streaming** (células, HLOD, LOD, orçamento de memória, preview de carregamento)
  - **Editor de Trânsito e Multidões** (faixas, cruzamentos, semáforos, spawning, densidade)
  - **Editor de Veículos** (suspensão, motor, câmbio, pneus, dano, câmera)
  - **Editor de Tempo/Clima** (hora, sol, nuvens, chuva, neve, vento)
  - **Profiler visual** (CPU/GPU/memória, frame time por sistema, tempos por passe)
  - **Render Graph Debugger** (passes, dependências, recursos, overdraw, HZB)
  - **Debugger de física** (colliders, ragdoll, raycasts)
  - **Debugger de animação** (esqueleto, IK, blend weights)
  - **Debugger de áudio** (vozes, buses, oclusão, latência)
  - **Painel de Documentação** (dockable, navegável dentro do editor — ver seção 5.6)

### 5.2 Atalhos de teclado

Conjunto padrão de produtividade, customizável em "Edit → Atalhos de Teclado":

- `Ctrl+S` salvar cena · `Ctrl+Z/Y` desfazer/refazer
- `Ctrl+D` duplicar entity · `Delete` deletar
- `F` focar câmera no objeto selecionado
- `W/E/R` mover/rotacionar/escalar (gizmo)
- `Ctrl+P` entrar/sair do Play Mode
- `F1` abrir documentação contextual

### 5.3 Drag and drop

Requisito funcional obrigatório em todo o editor:

- Arrastar `.glb` do Asset Browser para o Viewport → cria Entity com Mesh Renderer configurado
- Arrastar textura para um slot de material no Inspector → atribui a textura
- Arrastar script C#/nó para uma Entity → adiciona como componente
- Arrastar Entity sobre outra na Hierarquia → parenting
- Campos de referência no Inspector aceitam arrastar Entities/assets compatíveis

### 5.4 Scripting Visual (nós)

- Modelo de **execução por fios**, no estilo Unreal Blueprint: nós conectados por **pinos de execução** (fio de ordem/fluxo) e **pinos de dados** (fio tipado, carrega valores)
- Categorias de nós: Eventos (`OnBeginPlay`, `OnTakeDamage`, `OnCollision`), Ações (`PlayAnimation`, `ApplyDamage`, `SpawnEntity`), Fluxo de controle (`Branch`, `Sequence`, `ForLoop`, `Delay`), Variáveis, Funções customizadas (sub-grafos reutilizáveis)
- **Diferencial técnico**: ao salvar, o grafo é traduzido para **C# real e legível**, compilado pelo Roslyn — não é interpretado nó-por-nó em runtime como a maioria dos visual scriptings. Resultado: performance de C# nativo.
- Debug: visualização de valores fluindo pelos pinos em tempo real durante Play Mode, breakpoints em nós
- Escopo: editor visual serve para **qualquer lógica de gameplay**, não apenas IA de boss

### 5.5 Sistema de validação contextual (avisos)

Inspirado no modelo do Godot:

- Ícone de aviso (⚠) na Hierarquia quando uma Entity tem configuração incompleta/inconsistente
- Banner de aviso no Inspector explicando o problema (ex.: "Mesh Renderer sem material atribuído")
- Avisos também registrados no Console
- Comando "Validar Cena" roda a checagem em todas as Entities de uma vez
- Cada componente define suas próprias regras de validação — padrão arquitetural extensível conforme novos sistemas são adicionados

### 5.6 Documentação

- **Fonte única**: gerada a partir dos comentários XML (`/// <summary>`) do próprio código C# da API
- **Dentro do editor**: painel dockable de documentação completa e pesquisável (estilo Godot), navegação por links cruzados entre tipos, atalho `F1` contextual (clicar num nó/tipo abre a página de referência correspondente)
- **Fora do editor**: mesmo conteúdo publicado como site estático, para consulta sem abrir o editor ou compartilhamento de link

### 5.7 Sistema de Build

- Configuração de build dentro do Project Settings
- Botão "Build" gera o executável final em 1 clique: copia o executável base já linkado, empacota assets e C# e inclui apenas as `.dll` opcionais realmente usadas pelo projeto (ver seção 2.1)

### 5.8 UI (HUD e menus do jogo)

- Dear ImGui: exclusivo para ferramentas internas do editor
- Framework de UI próprio (retained mode): toolkit que o dev usa para construir a UI do jogo final (HUD, menus) — não é uma UI pronta, é a ferramenta de construção

---

## 6. Launcher e fluxo de criação de projeto

### 6.1 Kizuri Launcher

Aplicativo separado do Editor, executado antes dele:

- Lista de projetos recentes
- Botão "Novo Projeto" (sempre cria projeto em branco — sem templates)
- Gerencia versão da engine instalada, evitando incompatibilidade entre projeto e engine

### 6.2 Estrutura gerada ao criar um projeto

```
MeuJogo/
├── Assets/          (modelos glTF, texturas, áudio)
├── Scripts/         (C# puro + grafos de nós)
├── Scenes/
├── Config/
├── Build/           (saída do sistema de build)
└── MeuJogo.kzproj   (aponta para a versão da engine usada)
```

### 6.3 Preferências do editor (layout de painéis)

- Layout de painéis (equivalente ao `imgui.ini`) **não fica salvo dentro da pasta do projeto**
- Salvo por usuário, em local padronizado pelo instalador (ex.: `%APPDATA%/Kizuri/EditorLayout.ini`)
- Resultado: a pasta do projeto permanece limpa, contendo apenas Assets/Scripts/Scenes/Config; o layout é preferência pessoal do dev, não algo versionado no projeto

---

## 7. Requisitos de hardware

**Metodologia**: como não é viável testar fisicamente em todo hardware existente, a estimativa segue a prática padrão da indústria:

1. Uso do Steam Hardware Survey como base do que a maioria dos jogadores realmente possui
2. Cálculo de orçamento de frame (16,6ms para 60fps), dividido entre renderer, física, animação etc.
3. Definição de uma "vertical slice" (cena representativa) testada em GPUs de referência de cada faixa de mercado
4. Extrapolação matemática entre pontos de referência usando benchmarks públicos (TechPowerUp, etc.)
5. Refinamento pós-lançamento via telemetria real de uso

**Esta tabela é uma meta de design inicial**, a ser validada quando existir conteúdo de jogo real (vertical slice) para benchmark:

| | Piso de compatibilidade | Mínimo de produção | Recomendado |
|---|---|---|---|
| GPU | GT 610 / integrada antiga (720p, preset baixo) | GTX 1060 6GB / RX 580 (1080p/30 com FSR) | RTX 3060 / RX 6600 (1080p/60 ou 1440p com FSR) |
| CPU | Core 2 Duo / Phenom II (piso de teste) | 4 núcleos / 8 threads (ex.: i5-8400) | 6 núcleos / 12 threads (ex.: Ryzen 5 5600) |
| RAM | 4 GB (piso de teste) | 8 GB | 16 GB |
| Armazenamento | HD (piso de teste) | SSD (essencial para streaming sem stutter) | SSD NVMe |
| Alvo de resolução/FPS | 720p / 30fps, preset baixo | 1080p / 30fps com FSR | 1080p / 60fps ou 1440p com FSR |

O **piso de compatibilidade** existe para provar que a engine não tem custo patológico e para o autor validar no E5700/GT 610. Ele **não é o alvo visual**: fidelidade, sombras, pós-processamento e densidade de cidade são medidas no hardware de produção e no recomendado. A GT 610 não pode ditar o teto gráfico da engine.

O requisito mínimo de 4 núcleos reais é arquitetural, não arbitrário: é o piso necessário para separar efetivamente a thread de UI do trabalho pesado via job system (seção 4.12).

---

## 8. Riscos Técnicos e Decisões de Mitigação

Esta seção documenta armadilhas de baixo nível identificadas por revisão técnica externa, que não são visíveis ao planejar a engine em alto nível, mas que comprometem a arquitetura se ignoradas na fundação. Cada risco tem uma decisão de mitigação já registrada, para ser aplicada na fase correspondente do Roadmap.

### 8.1 Fronteira C++/C# e Gerenciamento de Memória

- **Marshalling no hot path**: chamadas P/Invoke repetidas milhares de vezes por frame (ex.: posição de cada entidade) destroem performance. **Mitigação**: usar Blittable Types e ponteiros brutos (`void*`/`IntPtr`, `unsafe`/`ref struct`) para o C# ler/escrever direto na memória do ECS em C++, sem cópia na fronteira.
- **GC do .NET causando stutter**: alocação dinâmica (`class`, strings) na API C# dispara coletas de 10-50ms. **Mitigação**: API C# exposta pela engine usa apenas `struct`/tipos value, evitando alocação no hot path; pooling para qualquer alocação inevitável.
- **Alinhamento de memória (Blittable Types)**: `struct` em C++ e C# precisam ter layout de bytes idêntico, ou os dados corrompem na leitura. **Mitigação**: todo tipo compartilhado na fronteira C++/C# usa `[StructLayout(LayoutKind.Sequential)]` explícito e é validado por teste automatizado de tamanho/offset no CI.
- **Referências a entidades destruídas**: script C# segurando referência a uma Entity já deletada no C++ causa Access Violation. **Mitigação**: referências de Entity expostas ao C# usam um sistema de handle com geração (generational index), não ponteiro cru — acessar um handle inválido retorna erro controlado, não crash.
- **Fragmentação de heap por Hot-Reload**: recarregar assemblies via Roslyn sem limpar referências antigas infla a RAM continuamente. **Mitigação**: cada hot-reload usa um novo `AssemblyLoadContext` colecionável (`isCollectible: true`), com descarregamento explícito do contexto anterior antes de carregar o novo.

### 8.2 Concorrência e Escalonamento de Threads

- **Guerra de threads (over-subscription)**: Jolt Physics tem job system próprio; rodar em paralelo ao enkiTS sem coordenação gera context switching excessivo. **Mitigação**: implementar a interface `Jolt::JobSystem` como um adaptador que submete os jobs do Jolt diretamente ao enkiTS — um único scheduler para toda a engine (ver Fase 6 do Roadmap).
- **Condições de corrida no ECS híbrido**: script C# lendo um componente enquanto a física em C++ escreve nele corrompe dados. **Mitigação**: acesso a componentes segue o modelo de jobs do ECS (leitura/escrita declarada por sistema), com o scheduler garantindo que sistemas com dependência de dados não rodem simultaneamente sobre o mesmo componente.
- **Starvation da thread principal por I/O**: descompressão zstd em massa pelo Job System pode congestionar o SSD e travar a thread principal esperando sincronização. **Mitigação**: fila de I/O com prioridade e limite de operações concorrentes, para não saturar o barramento de disco de uma vez.
- **Inversão de prioridade no enkiTS**: job de baixa prioridade retendo um mutex que um job de alta prioridade (render do frame atual) precisa. **Mitigação**: jobs críticos de frame (render, input) evitam mutex compartilhado com jobs de baixa prioridade; usar estruturas lock-free onde a dependência for inevitável.

### 8.3 Renderização, RHI (DX11) e Streaming de Assets

- **Bloqueio de frame por upload síncrono à GPU**: enviar texturas/meshes carregadas em background via `UpdateSubresource`/`Map`/`Unmap` pode travar a thread principal na hora do envio. **Mitigação**: uploads de GPU passam por uma fila assíncrona com staging buffers, distribuídos ao longo de vários frames em vez de um upload bloqueante único.
- **Stall de pipeline por readback de GPU**: ler um pixel da GPU (ex.: seleção de entidade por clique no Viewport) força a CPU a esperar toda a renderização terminar. **Mitigação**: readback usa buffers com N frames de atraso (a CPU lê o resultado do frame anterior, não do atual), evitando sincronização bloqueante.
- **Redundância de estado na RHI**: a camada fina pode reenviar Render States já ativos na GPU. **Mitigação**: a RHI mantém um cache do último estado enviado e descarta chamadas redundantes antes de repassar ao driver.
- **Gargalo de Draw Calls no Deferred Rendering**: cenas urbanas densas podem estourar o limite de draw calls do DX11 na CPU. **Mitigação**: instancing agressivo para geometria repetida (prédios, props, vegetação), culling de oclusão, meshlets/GPU culling e static batching desde o renderer, não como otimização posterior.
- **Falta de VRAM sem Texture Streaming**: cidade contínua sem streaming estoura a VRAM do hardware mínimo. **Mitigação**: streaming de textura por distância e por célula, orçamento de VRAM, priorização e despejo previsível, integrado ao pipeline de assets e ao mundo.
- **Textura do Viewport destruída no meio do frame**: redimensionar o renderer depois que o ImGui registrou a textura do painel deixa o Viewport cinza ou com o handle antigo. **Mitigação**: resize, render scale/FSR e resolução dinâmica são enfileirados e aplicados no início do frame seguinte, antes do `NewFrame`; nenhum target referenciado pelo draw list é destruído durante a montagem da UI.

### 8.4 Física (Jolt), Animação e Gameplay

- **Desalinhamento temporal entre física e renderização**: física em passo fixo (ex.: 60Hz) e renderização em taxa variável causam tremor visual. **Mitigação**: interpolação de posição/rotação do collider entre o último e o penúltimo passo de física, aplicada só na renderização (a simulação em si permanece no passo fixo).
- **Penetração de colisão em plataformas móveis**: Root Motion sobre elevadores rápidos pode atravessar o chão ou esmagar contra o teto. **Mitigação**: velocidade da plataforma é aplicada *antes* do cálculo de colisão do Character Controller no mesmo passo de física, não depois.
- **Latência de hitbox (frame de defasagem)**: um frame de atraso entre a animação do osso e o posicionamento do collider causa ataques fora de sincronia. **Mitigação**: colliders de ataque (hitboxes) são atualizados dentro do mesmo passo de animação, antes do passo de física do frame, não no frame seguinte.
- **Erro de arredondamento em mundos grandes (float 32-bit)**: precisão do DirectXMath degrada longe da origem, causando trepidação e falhas de colisão. **Mitigação**: sistema de "Origin Rebasing" — o mundo é reancorado periodicamente perto do jogador (a origem (0,0,0) se move com o personagem, e tudo mais é recalculado em relação a ela), evitando coordenadas absolutas muito grandes.

### 8.5 Engenharia do Editor

- **Corrupção de estado no Play-in-Editor**: rodar o jogo no mesmo espaço de memória do editor pode destruir a cena original ao editar scripts. **Mitigação**: o estado da cena é serializado (via o mesmo sistema de save da seção 4.10) antes de entrar em Play Mode, e restaurado automaticamente ao sair — não é um reload do arquivo do zero, é snapshot em memória.
- **Condição de corrida no Inspector (Immediate Mode)**: editar um valor no ImGui enquanto o Job System altera o mesmo dado em background gera conflito de escrita. **Mitigação**: edições feitas pelo Inspector são enfileiradas e aplicadas no início do frame seguinte, no mesmo ponto de sincronização que o restante do ECS usa — nunca escrita direta e imediata durante o frame.
- **Assets binários (FlatBuffers) incompatíveis com merge no Git**: cenas/terrenos binários impedem merge de branches. **Mitigação**: não é um problema ativo no momento (fluxo de trabalho é de um único desenvolvedor), mas registrado como risco latente — se o projeto ganhar mais de um colaborador direto no futuro, será necessário um formato intermediário textual (ex.: JSON) para cenas, convertido para binário só no build.
- **Estouro de RAM no Undo/Redo**: salvar cópias completas de componentes a cada alteração de mouse (pintura de vegetação, sculpting) esgota memória rápido. **Mitigação**: histórico de undo usa diffs incrementais (delta de mudança), não snapshots completos, com compactação periódica do histórico antigo.
- **Travamento por recompilação do Roslyn a cada Ctrl+S**: compilar o grafo inteiro no disco a cada salvamento trava a interface por segundos. **Mitigação**: compilação incremental (só o(s) grafo(s) modificado(s)) rodando em background pelo Job System, sem bloquear a UI; o editor mostra estado "compilando" sem travar.

### 8.6 IA, Rede e Sistemas Auxiliares

- **Gargalo de CPU em NavMesh 3D vertical**: recalcular NavMesh dinamicamente em áreas com múltiplos andares sobrepostos (comum em cidade e em souls-like) derruba o frame rate. **Mitigação**: recálculo de NavMesh é particionado em tiles e só a região afetada por uma mudança (ex.: portão abrindo, carro bloqueando) é recalculada, nunca o mundo inteiro.
- **Divergência de estado em rede P2P**: sem previsão de cliente e rebobinamento, perda de pacotes causa descompasso entre o que o jogador vê e o que o host aplica. **Mitigação**: client-side prediction com reconciliação (rollback) no sistema de rede, a ser detalhado tecnicamente na fase de rede do Roadmap antes da implementação.
- **Latência de input pela fila de mensagens do Win32**: processar esquiva, ataque ou direção de veículo dentro do loop padrão `GetMessage` expõe o input a oscilações do SO/renderização. **Mitigação**: leitura de input crítico (esquiva, ataque, aceleração, freio) via polling de estado de baixo nível (ex.: `GetAsyncKeyState`/Raw Input) fora do loop de mensagens do Win32, não dependente da fila de eventos da janela.

### 8.7 Streaming de mundo aberto

- **Hitch na troca de célula**: carregar/descarregar geometria, texturas e áudio no meio do frame trava a experiência. **Mitigação**: carregamento assíncrono com orçamento por frame, fila de prioridade, upload de GPU distribuído e transição por HLOD antes da célula detalhada.
- **Popping visível de LOD/HLOD**: troca de malha ou textura perceptível. **Mitigação**: transições por distância com histerese, dithering/fade e calibração por cena de referência.
- **Vazamento de memória/VRAM**: células antigas mantidas por referência. **Mitigação**: ownership explícito, pools, contagem de referências, despejo auditável e telemetria por célula.
- **Costura entre células**: buracos, z-fighting ou iluminação diferente na divisa. **Mitigação**: células com borda compartilhada, normais/tangentes consistentes, probes e lightmaps contínuos, teste visual de costura.

### 8.8 Veículos e trânsito

- **Tunneling em alta velocidade**: carro atravessa parede ou outro carro. **Mitigação**: CCD, substepping, colisores compostos e limites de velocidade testados.
- **Instabilidade de suspensão**: jitter, capotamento artificial ou resposta elástica. **Mitigação**: modelo de suspensão calibrado, damping, passo fixo e testes de frenagem/curva/impacto.
- **Custo de multidões e trânsito**: milhares de agentes derrubam CPU. **Mitigação**: LOD de IA, atualização por frequência, pooling, simulação em background e orçamento por célula.
- **Sincronização entre física e animação do veículo**: rodas, suspensão e corpo fora de fase. **Mitigação**: atualização no mesmo passo de física e interpolação consistente no render.

### 8.9 Consistência visual e temporal

- **Cor inconsistente entre Editor e jogo**: sRGB/linear, exposição ou tonemap diferentes. **Mitigação**: um único workflow de cor, calibração por cena de referência e teste de imagem.
- **Flicker de luz/sombra**: bias, precisão, cascatas ou jitter. **Mitigação**: bias automático, normal offset, cascatas estabilizadas, cache de sombra e teste temporal com câmera parada e em movimento. Casters repetem o alpha cutout, o dither de LOD e o deslocamento por height map do G-buffer para não gerar sombra de geometria que não existe na cena visível.
- **Ghosting/smearing de TAA**: falta de velocity por objeto ou histórico inválido. **Mitigação**: velocity buffer por objeto e por osso com histórico de transform e pose, paridade de tesselação no velocity e no deslocamento, rejeição de histórico por profundidade, fundo estático reprojetado de forma analítica sem jitter e validação em objetos animados.
- **Shader permutations fora de controle**: tempo de compilação e variantes inconsistentes. **Mitigação**: chaves explícitas, cache de shaders, compilação assíncrona e inventário de variantes.

---

## 9. Fora de escopo (v1)

Itens conscientemente excluídos do escopo inicial:

- Multiplataforma (console, Linux, macOS)
- Ray tracing / path tracing (limitação de DX11)
- Servidores dedicados
- Marketplace de plugins/assets de terceiros
- Templates de projeto no Launcher
- Localização multi-idioma (apenas preparação arquitetural)
- Modding com suporte nativo da engine
- Hot-reload de código C++
- Paridade feature-by-feature com RAGE, Unreal ou Unity
- Renderer alternativo (DX12/Vulkan) antes do backend DX11 estar maduro

Streaming de mundo aberto **não** está fora de escopo: ele é requisito do primeiro jogo urbano e será construído como sistema da engine.

---

## 10. Próximos passos

1. Auditar a baseline atual: componente Material, iluminação (flicker e rachaduras), viewport cinza no resize e tremulação residual do TAA.
2. Criar cenas de referência e testes de regressão visual/performance para renderer, áudio, física e animação.
3. Fechar a Fase 10 (animação) com state machine, camadas, IK, root motion, retargeting e eventos.
4. Executar a Fase 10.5 de estabilização crítica.
5. Iniciar a Fase 11 de renderização de produção.
6. Manter o padrão de um commit por correção verificada, CI verde e portões de qualidade da seção 2.5.
