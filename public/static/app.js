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
    leaderboard: [],
    config: { category: null, playerName: null, points: 0, isGuest: false }
};

const appDiv = document.getElementById('app');

window.addEventListener('popstate', (event) => {
    if (event.state && event.state.page === 'menu') renderMainMenu();
    else if (event.state && event.state.page === 'profile') renderProfile();
    else renderMainMenu();
});

window.addEventListener('load', () => {
    history.replaceState({ page: 'menu' }, "", window.location.pathname);
    initApp();
});

async function initApp() {
    try {
        const res = await fetch('/api/user/profile', {
            method: 'GET',
            credentials: 'same-origin'
        });

        if (res.ok) {
            const data = await res.json();
            state.config.playerName = data.username;
            state.config.points = data.total_points || 0;
            state.config.isGuest = data.is_guest || false;
        } else if (res.status === 401) {
            document.cookie = "SESSION=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=/;";
            state.config.playerName = null;
            state.config.points = 0;
            state.config.isGuest = false;
        }
    } catch (err) {
        console.warn("Could not fetch profile.", err);
    }
    renderMainMenu();
}

function transitionView(newHTML, callback = null) {
    appDiv.classList.add('view-hidden');
    setTimeout(() => {
        appDiv.innerHTML = newHTML;
        void appDiv.offsetWidth;
        appDiv.classList.remove('view-hidden');
        if (callback) callback();
    }, 250);
}

function getMainMenuHTML() {
    let lbHtml = '';
    if (state.leaderboard && state.leaderboard.length > 0) {
        state.leaderboard.forEach((user, idx) => {
            let rankStyle = idx === 0 ? 'text-warning fw-bold' : (idx === 1 ? 'text-light' : (idx === 2 ? 'text-info' : 'text-secondary'));
            lbHtml += `
            <div class="d-flex justify-content-between align-items-center mb-3 p-2 border-bottom border-secondary" style="background: rgba(0,0,0,0.2);">
                <div>
                    <span class="${rankStyle} me-3 code-font">#${idx + 1}</span>
                    <span class="text-light code-font">${user.username}</span>
                </div>
                <span class="badge bg-dark border border-secondary code-font">${user.points} pts</span>
            </div>`;
        });
    } else {
        lbHtml = `<p class="text-secondary small text-center code-font py-4">No ranked operators yet.</p>`;
    }

    return `
    <div class="row align-items-center g-5">
        <div class="col-lg-7 text-lg-start text-center">
            <div class="mb-3 d-flex justify-content-between align-items-center">
                <span class="badge rounded-pill border border-info text-info px-3 py-2 code-font">v0.1</span>
                ${state.config.playerName && !state.config.isGuest
                    ? `<button class="btn btn-sm btn-outline-info code-font" onclick="renderProfile()">Profile (${state.config.points} pts)</button>`
                    : `<button class="btn btn-sm btn-outline-info code-font px-3" onclick="renderAuth()">Login / Register</button>`
                }
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
                <div class="leaderboard-container">
                    ${lbHtml}
                </div>
            </div>
        </div>
    </div>
    `;
}

async function renderMainMenu() {
    try {
        const res = await fetch('/api/leaderboard', {
            method: 'GET',
            credentials: 'same-origin'
        });
        if (res.ok) {
            const data = await res.json();
            state.leaderboard = data.leaderboard || [];
        }
    } catch (err) {
        console.warn("Could not fetch leaderboard.", err);
    }
    transitionView(getMainMenuHTML());
}

async function renderProfile() {
    history.pushState({ page: 'profile' }, "", `#profile`);

    try {
        const res = await fetch('/api/user/stats', {
            method: 'GET',
            credentials: 'same-origin'
        });
        if (res.ok) {
            const data = await res.json();
            state.config.points = data.points || 0;
        }
    } catch (err) {
        console.warn("Failed to update stats on profile view.", err);
    }

    const html = `
    <div class="row justify-content-center py-5">
        <div class="col-md-6 col-lg-5">
            <div class="glass-panel p-4 p-md-5 shadow-lg position-relative text-center">
                <button onclick="renderMainMenu()" class="btn btn-sm btn-outline-secondary position-absolute top-0 start-0 m-3 code-font">&larr; Back</button>
                <h4 class="code-font text-light mb-3"><span class="text-warning">${state.config.playerName}</span></h4>
                <div class="bg-dark border border-secondary p-4 rounded mb-4">
                    <h1 class="display-4 fw-bold text-success mb-0">${state.config.points}</h1>
                    <span class="text-secondary small code-font">Points</span>
                </div>
                <button class="btn btn-outline-danger w-100 code-font fw-bold" onclick="logout()">Log Out</button>
            </div>
        </div>
    </div>`;
    transitionView(html);
}

function renderAuth() {
    const authHTML = `
    <div class="row justify-content-center py-5">
        <div class="col-md-6 col-lg-5">
            <div class="glass-panel p-4 p-md-5 shadow-lg position-relative">
                <button onclick="renderMainMenu()" class="btn btn-sm btn-outline-secondary position-absolute top-0 start-0 m-3 code-font">&larr; Back</button>
                <h2 class="fw-bold mb-1 text-center mt-3">Account</h2>

                <ul class="nav nav-tabs mt-4" id="authTabs" role="tablist">
                  <li class="nav-item" role="presentation">
                    <button class="nav-link active text-info code-font" id="login-tab" data-bs-toggle="tab" data-bs-target="#login" type="button" role="tab">Login</button>
                  </li>
                  <li class="nav-item" role="presentation">
                    <button class="nav-link text-info code-font" id="register-tab" data-bs-toggle="tab" data-bs-target="#register" type="button" role="tab">Register</button>
                  </li>
                </ul>

                <div class="tab-content mt-4" id="authTabContent">
                  <div class="tab-pane fade show active" id="login" role="tabpanel">
                    <form onsubmit="handleAuth(event, '/api/auth/login')">
                        <div class="mb-3">
                            <label class="form-label text-secondary small fw-bold">Username</label>
                            <input type="text" name="username" class="form-control bg-dark text-light border-secondary" required>
                        </div>
                        <div class="mb-4">
                            <label class="form-label text-secondary small fw-bold">Password</label>
                            <input type="password" name="password" class="form-control bg-dark text-light border-secondary" required>
                        </div>
                        <button type="submit" class="btn btn-info w-100 fw-bold code-font">Login &rarr;</button>
                    </form>
                  </div>

                  <div class="tab-pane fade" id="register" role="tabpanel">
                    <form onsubmit="handleAuth(event, '/api/auth/register')">
                        <div class="mb-3">
                            <label class="form-label text-secondary small fw-bold">New Username</label>
                            <input type="text" name="username" class="form-control bg-dark text-light border-secondary" required>
                        </div>
                        <div class="mb-4">
                            <label class="form-label text-secondary small fw-bold">Password</label>
                            <input type="password" name="password" class="form-control bg-dark text-light border-secondary" required>
                        </div>
                        <button type="submit" class="btn btn-warning w-100 fw-bold code-font text-dark">Register &rarr;</button>
                    </form>
                  </div>
                </div>
                <div id="authError" class="text-danger mt-3 small code-font text-center"></div>
            </div>
        </div>
    </div>`;
    transitionView(authHTML);
}

async function handleAuth(event, endpoint) {
    event.preventDefault();
    const form = event.target;
    const params = new URLSearchParams();
    params.append('username', form.elements.username.value);
    params.append('password', form.elements.password.value);

    try {
        const res = await fetch(endpoint, {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            credentials: 'same-origin',
            body: params.toString()
        });
        const data = await res.json();
        if (!res.ok) {
            document.getElementById('authError').innerText = data.error || "Authentication failed.";
            return;
        }
        state.config.playerName = data.username;
        state.config.points = data.total_points || 0;
        state.config.isGuest = false;
        renderMainMenu();
    } catch (err) {
        document.getElementById('authError').innerText = "Network error. Server offline.";
    }
}

async function logout() {
    document.cookie = "SESSION=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=/;";
    state.config.playerName = null;
    state.config.points = 0;
    state.config.isGuest = false;
    renderMainMenu();
}

function nextChallenge() {
    const available = state.challenges.filter(c => c.type === state.config.category);
    if (available.length === 0) {
        renderMainMenu();
        return;
    }
    state.currentChallenge = available[Math.floor(Math.random() * available.length)];
    renderChallenge(state.currentChallenge);
}

function startGame(category) {
    state.config.category = category;
    history.pushState({ page: 'setup', category: category }, "", `#setup-${category}`);

    if (state.config.playerName) {
        launchGameDirectly();
        return;
    }

    const setupHTML = `
<div class="row justify-content-center py-5">
    <div class="col-md-6 col-lg-5">
        <div class="glass-panel p-4 p-md-5 shadow-lg position-relative">
            <button onclick="renderMainMenu()" class="btn btn-sm btn-outline-secondary position-absolute top-0 start-0 m-3 code-font">&larr; Back</button>
            <h2 class="fw-bold mb-1 text-center mt-3">New Game</h2>
            <p class="text-center text-info code-font small mb-4">Category: <b class="text-uppercase">${category}</b></p>
            <form onsubmit="launchGuestGame(event)">
                <div class="mb-4">
                    <label class="form-label text-secondary small fw-bold">Guest Name</label>
                    <input type="text" id="playerName" class="form-control form-control-lg bg-dark text-light border-secondary" required autocomplete="off">
                </div>
                <div class="d-grid">
                    <button type="submit" class="btn btn-info btn-lg fw-bold code-font">Play as Guest &rarr;</button>
                </div>
            </form>
        </div>
    </div>
</div>`;
    transitionView(setupHTML, () => document.getElementById('playerName').focus());
}

async function launchGuestGame(event) {
    event.preventDefault();
    state.config.playerName = document.getElementById('playerName').value || 'Guest';
    state.config.isGuest = true;
    appDiv.innerHTML = `
        <div class="text-center py-5">
            <div class="spinner-border text-info mb-3" role="status"></div>
            <h3 class="code-font text-muted">Authenticating Session...</h3>
        </div>`;
    try {
        const authRes = await fetch('/api/auth/guest', {
            method: 'POST',
            credentials: 'same-origin'
        });
        if (!authRes.ok) throw new Error("Failed to authenticate guest.");
    } catch (err) {
        console.warn("Auth failed, continuing in offline mode.", err);
    }
    loadChallenges();
}

async function launchGameDirectly() {
    appDiv.innerHTML = `
        <div class="text-center py-5">
            <div class="spinner-border text-info mb-3" role="status"></div>
            <h3 class="code-font text-muted">Loading Data...</h3>
        </div>`;
    loadChallenges();
}

async function loadChallenges() {
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
                const shiftVal = parseInt(c.value, 10) - 1;
                bitmask |= (1 << shiftVal);
            });
            params.append(`q${q.id}`, bitmask);
        }
    });
    appDiv.innerHTML = `
        <div class="text-center py-5">
            <div class="spinner-border text-warning mb-3" role="status"></div>
            <h3 class="code-font text-muted">Awaiting backend validation...</h3>
            <p class="text-secondary code-font small">${params.toString()}</p>
        </div>`;
    try {
        const response = await fetch('/api/challenge/verify', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            credentials: 'same-origin',
            body: params.toString()
        });
        const result = await response.json();

        if (!response.ok) {
            let errorHtml = `
                <div class="row justify-content-center py-5">
                    <div class="col-md-6 text-center">
                        <div class="glass-panel p-5 shadow-lg">
                            <h2 class="text-danger fw-bold mb-3">Verification Failed</h2>
                            <p class="lead text-light mb-4">${result.error || "An unknown error occurred."}</p>
                            <button onclick="renderMainMenu()" class="btn btn-outline-info code-font px-4">Return to Main Menu</button>
                        </div>
                    </div>
                </div>`;
            transitionView(errorHtml);
            return;
        }

        // Add the earned points to local state
        if (result.total_earned > 0 && !state.config.isGuest) {
            state.config.points += result.total_earned;
        }

        let headerText = "Partial Success";
        let headerClass = "text-warning";
        if (result.total_earned === result.max_points) {
            headerText = "Challenge Mastered";
            headerClass = "text-success";
        } else if (result.total_earned === 0) {
            headerText = "Incorrect";
            headerClass = "text-danger";
        }

        let resultHtml = `
            <div class="row justify-content-center py-5">
                <div class="col-md-8">
                    <div class="glass-panel p-4 p-md-5 shadow-lg">
                        <h2 class="text-center ${headerClass} fw-bold mb-2">
                            ${headerText}
                        </h2>
                        <h4 class="text-center text-light mb-4 code-font">Score: ${result.total_earned} / ${result.max_points}</h4>
                        <div class="mb-5">`;

        if (result.results && Array.isArray(result.results)) {
            result.results.forEach(r => {
                const isCorrect = r.correct === true || r.correct === "true";
                resultHtml += `
                    <div class="p-3 mb-3 rounded border ${isCorrect ? 'border-success' : 'border-danger'} bg-dark" style="background: rgba(0,0,0,0.3);">
                        <div class="d-flex justify-content-between align-items-center mb-2">
                            <strong class="${isCorrect ? 'text-success' : 'text-danger'} code-font">
                                Question ${r.id}: ${isCorrect ? 'CORRECT' : 'INCORRECT'}
                            </strong>
                            <span class="badge ${isCorrect ? 'bg-success' : 'bg-danger'}">${r.earned} pts</span>
                        </div>
                        <p class="mb-0 small text-secondary font-monospace">
                            <span class="text-info">Explanation:</span> ${r.explanation}
                        </p>
                    </div>`;
            });
        }

        resultHtml += `
                        </div>
                        <div class="d-flex justify-content-center gap-3">
                            <button onclick="renderMainMenu()" class="btn btn-outline-info code-font px-4 fw-bold">Main Menu</button>
                            <button onclick="nextChallenge()" class="btn btn-info code-font px-4 fw-bold">Next Challenge &rarr;</button>
                        </div>
                    </div>
                </div>
            </div>`;

        transitionView(resultHtml);

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
