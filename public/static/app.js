const MAIN_MENU_HTML = `
    <div class="row align-items-center g-5">
        <div class="col-lg-7 text-lg-start text-center">
            <div class="mb-3">
                <span class="badge rounded-pill border border-info text-info px-3 py-2 code-font">v0.1</span>
            </div>
            <h1 class="display-3 fw-bold mb-3">Compu<span class="hex-accent">Guessr</span></h1>
            <p class="lead text-secondary mb-5 pe-lg-5">
                Examine raw data, decode assembly snippets, or find vulnerabilities before the time runs out.
            </p>
            <div class="row g-3">
                <div class="col-md-4">
                    <div class="card category-card h-100 p-3" onclick="startGame('protocol')">
                        <div class="card-body p-0 text-center text-lg-start">
                            <h4 class="fw-bold hex-accent mb-2">Protocol</h4>
                            <p class="text-secondary small mb-0">Packet captures, network layers, and hex decoding.</p>
                        </div>
                    </div>
                </div>
                <div class="col-md-4">
                    <div class="card category-card h-100 p-3" onclick="startGame('instruction')">
                        <div class="card-body p-0 text-center text-lg-start">
                            <h4 class="fw-bold text-danger mb-2">Instruction</h4>
                            <p class="text-secondary small mb-0">x86_64, ARM, opcode analysis and reverse engineering.</p>
                        </div>
                    </div>
                </div>
                <div class="col-md-4">
                    <div class="card category-card h-100 p-3" onclick="startGame('fuzzer')">
                        <div class="card-body p-0 text-center text-lg-start">
                            <h4 class="fw-bold text-warning mb-2">Fuzzer</h4>
                            <p class="text-secondary small mb-0">Spot the buffer overflow, segfaults, and memory leaks.</p>
                        </div>
                    </div>
                </div>
            </div>
        </div>
        <div class="col-lg-5">
            <div class="glass-panel p-4 shadow-lg">
                <div class="d-flex justify-content-between align-items-center mb-4">
                    <h4 class="mb-0 fw-bold">Leaderboard</h4>
                </div>
            </div>
        </div>
    </div>
`;

const MOCK_DB = {
  "version": "1.1",
  "challenges": [
    {
      "id": "instruction_001",
      "type": "instruction",
      "difficulty": "easy",
      "title": "Hello, world!",
      "description": "Analyze this basic assembly snippet.",
      "codeblock": [
        "section .data",
        "msg     db \"Hello, world!\"",
        "len     equ $ - msg",
        "",
        "section .text",
        "global _start",
        "",
        "_start:",
        "    mov rax, 1",
        "    mov rdi, 1",
        "    mov rsi, msg",
        "    mov rdx, len",
        "    syscall"
      ],
      "questions": [
        {
          "id": 1,
          "question": "What is the output of this program?",
          "type": "text",
          "points": 25,
          "ui_hints": {"placeholder": "Exact output..."}
        },
        {
          "id": 2,
          "question": "Which operating system is this written for?",
          "type": "multiple_response",
          "points": 25,
          "responses": [
            {"id": 1, "text": "Windows NT"},
            {"id": 2, "text": "Linux (x86_64)"},
            {"id": 3, "text": "FreeBSD"}
          ]
        }
      ]
    }
  ]
};

const state = {
    challenges: [],
    currentChallenge: null,
    config: { category: null, playerName: 'Anon' }
};

const appDiv = document.getElementById('app');

window.addEventListener('popstate', (event) => {
    if (event.state && event.state.page === 'menu') renderMainMenu();
    else renderMainMenu();
});

window.addEventListener('load', () => {
    history.replaceState({ page: 'menu' }, "", window.location.pathname);
    renderMainMenu();
});

function transitionView(newHTML, callback = null) {
    appDiv.classList.add('view-hidden');
    setTimeout(() => {
        appDiv.innerHTML = newHTML;
        void appDiv.offsetWidth;
        appDiv.classList.remove('view-hidden');
        if (callback) callback();
    }, 250);
}

function renderMainMenu() {
    transitionView(MAIN_MENU_HTML);
}

function startGame(category) {
    state.config.category = category;
    history.pushState({ page: 'setup', category: category }, "", `#setup-${category}`);

    const setupHTML = `
<div class="row justify-content-center py-5">
    <div class="col-md-6 col-lg-5">
        <div class="glass-panel p-4 p-md-5 shadow-lg position-relative">
            <button onclick="renderMainMenu()" class="btn btn-sm btn-outline-secondary position-absolute top-0 start-0 m-3 code-font">&larr; Back</button>
            <h2 class="fw-bold mb-1 text-center mt-3">New Game</h2>
            <p class="text-center text-info code-font small mb-4">Category: <b class="text-uppercase">${category}</b></p>
            <form id="setupForm" onsubmit="launchGame(event)">
                <div class="mb-4">
                    <label class="form-label text-secondary small fw-bold">Guest Name</label>
                    <input type="text" id="playerName" class="form-control form-control-lg bg-dark text-light border-secondary" required autocomplete="off">
                </div>
                <div class="d-grid">
                    <button type="submit" class="btn btn-info btn-lg fw-bold code-font">Start &rarr;</button>
                </div>
            </form>
        </div>
    </div>
</div>`;
    transitionView(setupHTML, () => document.getElementById('playerName').focus());
}

async function launchGame(event) {
    event.preventDefault();
    state.config.playerName = document.getElementById('playerName').value || 'Anon';
    appDiv.innerHTML = `
        <div class="text-center py-5">
            <div class="spinner-border text-info mb-3" role="status"></div>
            <h3 class="code-font text-muted">Fetching challenges...</h3>
        </div>`;
    try {
        const res = await fetch('/challenges/challenges.json');
        if (!res.ok) throw new Error("Network response was not ok");
        const data = await res.json();
        state.challenges = data.challenges;
    } catch (err) {
        console.warn("Using mock DB. Server missing or CORS error.", err);
        state.challenges = MOCK_DB.challenges;
    }
    const available = state.challenges.filter(c => c.type === state.config.category);
    if (available.length === 0) {
        appDiv.innerHTML = `<div class="text-center py-5"><h3 class="text-danger">No Challenges Found</h3><p>Could not find any challenges for ${state.config.category}.</p><button onclick="renderMainMenu()" class="btn btn-outline-light">Return</button></div>`;
        return;
    }
    state.currentChallenge = available[Math.floor(Math.random() * available.length)];
    renderChallenge(state.currentChallenge);
}

function renderChallenge(chal) {
    let html = `
    <div class="row justify-content-center py-4">
        <div class="col-lg-8">
            <div class="glass-panel p-4 shadow-lg position-relative">
                <div class="d-flex justify-content-between align-items-center mb-3">
                    <span class="badge bg-secondary text-uppercase code-font">${chal.difficulty}</span>
                    <span class="code-font text-muted small">ID: ${chal.id}</span>
                </div>
                <h2 class="fw-bold text-info mb-2">${chal.title}</h2>
                <p class="text-light mb-4">${chal.description}</p>`;
    if (chal.codeblock && chal.codeblock.length > 0) {
        html += `<div class="bg-dark p-3 rounded mb-4" style="border-left: 3px solid #38bdf8; overflow-x: auto;">
                    <pre class="mb-0"><code class="code-font text-light" style="font-size: 0.9rem;">${chal.codeblock.join('\n')}</code></pre>
                 </div>`;
    }
    html += `<form id="submitForm" onsubmit="submitAnswers(event)">`;
    chal.questions.forEach((q, index) => {
        html += `<div class="mb-4 p-3 border border-secondary rounded" style="background: rgba(0,0,0,0.2);">
                    <div class="d-flex justify-content-between align-items-center mb-2">
                        <h5 class="mb-0">Q${index + 1}: ${q.question}</h5>
                        <span class="badge bg-dark border border-secondary">${q.points} pts</span>
                    </div>`;
        if (q.type === 'text') {
            const placeholder = q.ui_hints?.placeholder || "Enter answer...";
            html += `<input type="text" name="q${q.id}" class="form-control bg-dark text-light border-secondary mt-3 code-font" placeholder="${placeholder}" required>`;
        } else if (q.type === 'multiple_choice') {
            html += `<div class="mt-3">`;
            q.responses.forEach(r => {
                html += `<div class="form-check mb-2">
                            <input class="form-check-input" type="radio" name="q${q.id}" id="q${q.id}_r${r.id}" value="${r.id}" required>
                            <label class="form-check-label code-font" for="q${q.id}_r${r.id}">${r.text}</label>
                         </div>`;
            });
            html += `</div>`;
        } else if (q.type === 'multiple_response') {
            html += `<div class="mt-3">`;
            q.responses.forEach(r => {
                // Notice the data-question attribute used for querying the bitmask later
                html += `<div class="form-check mb-2">
                            <input class="form-check-input" type="checkbox" data-question="q${q.id}" id="q${q.id}_r${r.id}" value="${r.id}">
                            <label class="form-check-label code-font" for="q${q.id}_r${r.id}">${r.text}</label>
                         </div>`;
            });
            html += `</div>`;
        }
        html += `</div>`;
    });
    html += `   <div class="d-flex justify-content-between align-items-center mt-4">
                    <button type="button" class="btn btn-outline-secondary code-font" onclick="renderMainMenu()">Abort</button>
                    <button type="submit" class="btn btn-info fw-bold code-font px-5">Submit Payload &rarr;</button>
                </div>
            </form>
            </div>
        </div>
    </div>`;

    transitionView(html);
}

async function submitAnswers(event) {
    event.preventDefault();
    const form = event.target;
    const params = new URLSearchParams();
    params.append('id', state.currentChallenge.id);
    state.currentChallenge.questions.forEach(q => {
        if (q.type === 'text' || q.type === 'multiple_choice') {
            const el = form.elements[`q${q.id}`];
            if (el && el.value) {
                params.append(`q${q.id}`, el.value);
            }
        } else if (q.type === 'multiple_response') {
            const checks = form.querySelectorAll(`input[data-question="q${q.id}"]:checked`);
            let bitmask = 0;
            checks.forEach(c => {
                const shiftVal = parseInt(c.value, 10);
                bitmask |= (1 << shiftVal);
            });
            params.append(`q${q.id}`, bitmask);
        }
    });

    console.log("Submitting payload:", params.toString());

    appDiv.innerHTML = `
        <div class="text-center py-5">
            <div class="spinner-border text-warning mb-3" role="status"></div>
            <h3 class="code-font text-muted">Awaiting backend validation...</h3>
            <p class="text-secondary code-font small">${params.toString()}</p>
        </div>`;

    try {
        const response = await fetch('/api/submit', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: params.toString()
        });

        const result = await response.json();
        alert("Backend replied: " + JSON.stringify(result));
        renderMainMenu();

    } catch (err) {
        console.error(err);
        appDiv.innerHTML = `
            <div class="text-center py-5">
                <h3 class="text-danger">Backend Offline</h3>
                <p>The FastCGI engine did not respond.</p>
                <button onclick="renderMainMenu()" class="btn btn-outline-light">Return to Main Menu</button>
            </div>`;
    }
}
