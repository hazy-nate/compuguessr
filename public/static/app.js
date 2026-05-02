const MOCK_DB = {
  "version": "1.1",
  "challenges": []
};

const state = {
    challenges: [],
    currentChallenge: null,
    leaderboard: [],
    config: { category: null, playerName: null, points: 0, isGuest: false, description: "", completedCount: 0, completedHashes: [] }
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

function hashStr(str) {
    let hash = 5381n;
    for (let i = 0; i < str.length; i++) {
        hash = ((hash << 5n) + hash) + BigInt(str.charCodeAt(i));
    }
    return hash.toString();
}

function escapeHTML(str) {
    if (!str) return "";
    return str.toString().replace(/[&<>'"]/g, tag => ({
        '&': '&amp;',
        '<': '&lt;',
        '>': '&gt;',
        "'": '&#39;',
        '"': '&quot;'
    }[tag] || tag));
}

function sanitizeHTML(str) {
    if (!str) return "";
    if (typeof DOMPurify !== 'undefined') {
        return DOMPurify.sanitize(str, {
            ADD_TAGS: ['iframe', 'img'],
            ADD_ATTR: ['allow', 'allowfullscreen', 'frameborder', 'scrolling', 'src', 'alt', 'width', 'height', 'target']
        });
    }
    return escapeHTML(str);
}

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
            state.config.description = data.description || "";
            state.config.completedCount = data.completed_count || 0;
            state.config.completedHashes = data.completed_hashes || [];
        } else if (res.status === 401) {
            document.cookie = "SESSION=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=/;";
            state.config.playerName = null;
            state.config.points = 0;
            state.config.isGuest = false;
        }
    } catch (err) {}
    renderMainMenu();
}

function transitionView(newHTML, callback = null) {
    appDiv.classList.add('view-hidden');
    setTimeout(() => {
        appDiv.innerHTML = newHTML;
        void appDiv.offsetWidth;
        appDiv.classList.remove('view-hidden');
        if (callback) callback();
    }, 400);
}

function getMainMenuHTML() {
    let lbHtml = '';
    if (state.leaderboard && state.leaderboard.length > 0) {
        lbHtml += `<table class="table-custom"><tbody>`;
        state.leaderboard.forEach((user, idx) => {
            let rankClass = idx === 0 ? 'rank-1' : (idx === 1 ? 'rank-2' : (idx === 2 ? 'rank-3' : 'text-secondary'));
            lbHtml += `
            <tr>
                <td class="code-font ${rankClass}" style="width: 15%;">0${idx + 1}</td>
                <td><span class="code-font text-light" style="cursor:pointer;" onclick="renderPublicProfile('${escapeHTML(user.username)}')">${escapeHTML(user.username)}</span></td>
                <td class="text-end code-font hex-accent">${user.points}</td>
            </tr>`;
        });
        lbHtml += `</tbody></table>`;
    } else {
        lbHtml = `<p class="text-secondary small code-font py-4">NO PLAYERS</p>`;
    }

    return `
    <div class="row mb-5 pb-4 border-bottom" style="border-color: rgba(255,255,255,0.05) !important;">
        <div class="col-12 d-flex justify-content-between align-items-center">
            <span class="code-font text-secondary" style="letter-spacing: 0.2em;">CG.01</span>
            ${state.config.playerName && !state.config.isGuest
                ? `<button class="btn btn-outline-info" onclick="renderProfile()">${escapeHTML(state.config.playerName)} [${state.config.points}]</button>`
                : `<button class="btn btn-outline-info" onclick="renderAuth()">LOG IN / SIGN UP</button>`
            }
        </div>
    </div>

    <div class="row g-5 flex-grow-1">
        <div class="col-xl-8 d-flex flex-column">
            <h1 class="display-1 fw-bold mb-4" style="line-height: 1;">COMPU<span class="hex-accent">.</span>GUESSR</h1>
            <p class="text-secondary mb-5 w-75" style="font-size: 1.1rem;">
                Examine raw data, decode assembly snippets, and locate vulnerabilities.
                A gamified technical analysis platform.
            </p>

            <div class="grid-categories">
                <div class="category-card" onclick="startGame('protocol')">
                    <h3>Protocol</h3>
                    <p class="small mb-0 code-font">PACKET CAPTURES / HEX DECODING / LAYER ANALYSIS</p>
                </div>
                <div class="category-card" onclick="startGame('instruction')">
                    <h3>Instruction</h3>
                    <p class="small mb-0 code-font">OPCODES / REGISTERS / REVERSE ENGINEERING</p>
                </div>
                <div class="category-card" onclick="startGame('fuzzer')">
                    <h3>Fuzzer</h3>
                    <p class="small mb-0 code-font">BUFFER OVERFLOWS / CORRUPTION / EXPLOITS</p>
                </div>
            </div>
        </div>

        <div class="col-xl-4">
            <div class="glass-panel p-5 h-100">
                <h4 class="mb-5 code-font" style="font-size: 0.9rem; letter-spacing: 0.2em; color: #888;">LEADERBOARD</h4>
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
    } catch (err) {}
    transitionView(getMainMenuHTML());
}

async function renderProfile() {
    history.pushState({ page: 'profile' }, "", `#profile`);
    try {
        const res = await fetch('/api/user/profile', {
            method: 'GET',
            credentials: 'same-origin'
        });
        if (res.ok) {
            const data = await res.json();
            state.config.points = data.total_points || 0;
            state.config.description = data.description || "";
            state.config.completedCount = data.completed_count || 0;
            state.config.completedHashes = data.completed_hashes || [];
        }
    } catch (err) {}

    const html = `
    <div class="row mb-5 pb-4 border-bottom" style="border-color: rgba(255,255,255,0.05) !important;">
        <div class="col-12">
            <button onclick="renderMainMenu()" class="btn btn-outline-info border-0 px-0">&larr; BACK</button>
        </div>
    </div>
    <div class="row justify-content-center">
        <div class="col-lg-6">
            <div class="glass-panel p-5">
                <h2 class="mb-1">${escapeHTML(state.config.playerName)}</h2>
                <p class="code-font hex-accent mb-5">PROFILE</p>

                <div class="row g-4 mb-5">
                    <div class="col-6">
                        <p class="text-secondary code-font mb-1" style="font-size: 0.7rem;">TOTAL SCORE</p>
                        <h3 class="fw-bold mb-0">${state.config.points}</h3>
                    </div>
                    <div class="col-6">
                        <p class="text-secondary code-font mb-1" style="font-size: 0.7rem;">CHALLENGES CLEARED</p>
                        <h3 class="fw-bold mb-0">${state.config.completedCount}</h3>
                    </div>
                </div>

                <form onsubmit="updateDescription(event)" class="mb-5">
                    <div class="mb-4">
                        <label class="form-label code-font text-secondary" style="font-size: 0.7rem;">PUBLIC DESCRIPTION (HTML PERMITTED)</label>
                        <textarea id="profileDesc" class="form-control" rows="5" maxlength="2048">${escapeHTML(state.config.description)}</textarea>
                    </div>
                    <button type="submit" class="btn btn-info w-100 mb-2">UPDATE PROFILE</button>
                    <div id="profileError" class="text-success mt-2 small code-font text-center" style="min-height: 20px;"></div>
                </form>

                <button class="btn btn-danger w-100" class="text-success mt-2 small code-font text-center" style="min-height: 20px;" onclick="deleteAccount()">DELETE ACCOUNT</button>

                <div class="border-top pt-4 mt-4" style="border-color: rgba(255,255,255,0.05) !important;">
                    <button class="btn btn-outline-info w-100" style="color: #666; border-color: rgba(255,255,255,0.05);" onclick="logout()">LOG OUT</button>
                </div>
            </div>
        </div>
    </div>`;
    transitionView(html);
}

window.renderPublicProfile = async function(username) {
    history.pushState({ page: 'public_profile', user: username }, "", `#user-${username}`);

    try {
        const params = new URLSearchParams();
        params.append('username', username);
        const res = await fetch('/api/user/public', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: params.toString()
        });

        if (!res.ok) {
            transitionView(`
            <div class="row justify-content-center py-5">
                <div class="col-md-6 text-center">
                    <h3 class="text-secondary">DOSSIER NOT FOUND</h3>
                    <button onclick="renderMainMenu()" class="btn btn-outline-info mt-4">BACK</button>
                </div>
            </div>`);
            return;
        }

        const data = await res.json();
        const html = `
        <div class="row mb-5 pb-4 border-bottom" style="border-color: rgba(255,255,255,0.05) !important;">
            <div class="col-12">
                <button onclick="renderMainMenu()" class="btn btn-outline-info border-0 px-0">&larr; BACK</button>
            </div>
        </div>
        <div class="row justify-content-center">
            <div class="col-lg-6">
                <div class="glass-panel p-5">
                    <h2 class="mb-1">${escapeHTML(data.username)}</h2>
                    <p class="code-font hex-accent mb-5">PROFILE</p>

                    <div class="row g-4 mb-5 border-bottom pb-5" style="border-color: rgba(255,255,255,0.05) !important;">
                        <div class="col-6">
                            <p class="text-secondary code-font mb-1" style="font-size: 0.7rem;">TOTAL SCORE</p>
                            <h3 class="fw-bold mb-0">${data.total_points}</h3>
                        </div>
                        <div class="col-6">
                            <p class="text-secondary code-font mb-1" style="font-size: 0.7rem;">CHALLENGES CLEARED</p>
                            <h3 class="fw-bold mb-0">${data.completed_count}</h3>
                        </div>
                    </div>

                    <div>
                        <p class="text-secondary code-font mb-3" style="font-size: 0.7rem;">DESCRIPTION</p>
                        <div class="text-light" style="word-break: break-all; overflow-x: auto;">
                            ${sanitizeHTML(data.description) || "<span class='text-secondary code-font'>[ NO DATA PROVIDED ]</span>"}
                        </div>
                    </div>
                </div>
            </div>
        </div>`;
        transitionView(html);
    } catch (err) {
        renderMainMenu();
    }
};

async function updateDescription(event) {
    event.preventDefault();
    const desc = document.getElementById('profileDesc').value;
    const params = new URLSearchParams();
    params.append('description', desc);

    try {
        const res = await fetch('/api/user/profile/update', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            credentials: 'same-origin',
            body: params.toString()
        });
        if (res.ok) {
            state.config.description = desc;
            document.getElementById('profileError').innerText = "Success";
            document.getElementById('profileError').classList.replace("text-danger", "text-success");
        }
    } catch (err) {
        document.getElementById('profileError').innerText = "Failure";
        document.getElementById('profileError').classList.replace("text-success", "text-danger");
    }
}

function renderAuth() {
    const authHTML = `
    <div class="row justify-content-center py-5">
        <div class="col-md-5">
            <div class="glass-panel p-5">
                <button onclick="renderMainMenu()" class="btn btn-outline-info border-0 px-0 mb-4">&larr; BACK</button>
                <h2 class="mb-4">LOG IN / SIGN UP</h2>

                <ul class="nav nav-pills mb-5 border-bottom pb-3" id="authTabs" role="tablist" style="border-color: rgba(255,255,255,0.05) !important;">
                  <li class="nav-item me-3" role="presentation">
                    <button class="nav-link active code-font text-light bg-transparent p-0 hex-accent" id="login-tab" data-bs-toggle="pill" data-bs-target="#login" type="button" role="tab">LOGIN</button>
                  </li>
                  <li class="nav-item" role="presentation">
                    <button class="nav-link code-font text-secondary bg-transparent p-0" id="register-tab" data-bs-toggle="pill" data-bs-target="#register" type="button" role="tab" onclick="document.getElementById('login-tab').classList.remove('hex-accent'); this.classList.add('hex-accent');">REGISTER</button>
                  </li>
                </ul>

                <div class="tab-content" id="authTabContent">
                  <div class="tab-pane fade show active" id="login" role="tabpanel">
                    <form onsubmit="handleAuth(event, '/api/auth/login')">
                        <div class="mb-4">
                            <label class="form-label code-font text-secondary" style="font-size: 0.7rem;">USERNAME</label>
                            <input type="text" name="username" class="form-control" autocomplete="off" required>
                        </div>
                        <div class="mb-5">
                            <label class="form-label code-font text-secondary" style="font-size: 0.7rem;">PASSWORD</label>
                            <input type="password" name="password" class="form-control" autocomplete="off" required>
                        </div>
                        <button type="submit" class="btn btn-info w-100">SUBMIT</button>
                    </form>
                  </div>

                  <div class="tab-pane fade" id="register" role="tabpanel">
                    <form onsubmit="handleAuth(event, '/api/auth/register')">
                        <div class="mb-4">
                            <label class="form-label code-font text-secondary" style="font-size: 0.7rem;">USERNAME</label>
                            <input type="text" name="username" class="form-control" autocomplete="off" required>
                        </div>
                        <div class="mb-5">
                            <label class="form-label code-font text-secondary" style="font-size: 0.7rem;">PASSWORD</label>
                            <input type="password" name="password" class="form-control" autocomplete="off" required>
                        </div>
                        <button type="submit" class="btn btn-info w-100">SIGN UP</button>
                    </form>
                  </div>
                </div>
                <div id="authError" class="text-danger mt-4 code-font text-center" style="font-size: 0.8rem; min-height: 20px;"></div>
            </div>
        </div>
    </div>`;
    transitionView(authHTML);

    setTimeout(() => {
        document.getElementById('login-tab').addEventListener('click', function() {
            this.classList.add('hex-accent');
            this.classList.replace('text-secondary', 'text-light');
            const reg = document.getElementById('register-tab');
            reg.classList.remove('hex-accent');
            reg.classList.replace('text-light', 'text-secondary');
        });
        document.getElementById('register-tab').addEventListener('click', function() {
            this.classList.add('hex-accent');
            this.classList.replace('text-secondary', 'text-light');
            const log = document.getElementById('login-tab');
            log.classList.remove('hex-accent');
            log.classList.replace('text-light', 'text-secondary');
        });
    }, 500);
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
            document.getElementById('authError').innerText = data.error ? data.error.toUpperCase() : "AUTHENTICATION FAILED.";
            return;
        }
        state.config.playerName = data.username;
        state.config.points = data.total_points || 0;
        state.config.isGuest = false;
        initApp();
    } catch (err) {
        document.getElementById('authError').innerText = "NETWORK ERROR.";
    }
}

async function logout() {
    document.cookie = "SESSION=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=/;";
    state.config.playerName = null;
    state.config.points = 0;
    state.config.isGuest = false;
    state.config.completedHashes = [];
    state.config.completedCount = 0;
    renderMainMenu();
}

function selectNextChallenge() {
    const available = state.challenges.filter(c => c.type === state.config.category);
    if (available.length === 0) return null;

    let uncompleted = available.filter(c => !state.config.completedHashes.includes(hashStr(c.id)));

    if (uncompleted.length === 0) {
        return "COMPLETE";
    }

    return uncompleted[Math.floor(Math.random() * uncompleted.length)];
}

function nextChallenge() {
    const chal = selectNextChallenge();
    if (!chal) {
        renderMainMenu();
        return;
    }
    if (chal === "COMPLETE") {
        const completeHtml = `
            <div class="row justify-content-center align-items-center flex-grow-1 py-5">
                <div class="col-md-6 text-center">
                    <h2 class="hex-accent mb-4">CATEGORY SECURED</h2>
                    <p class="text-secondary mb-5">All available vectors in ${escapeHTML(state.config.category).toUpperCase()} have been neutralized.</p>
                    <button onclick="state.config.completedHashes = []; nextChallenge()" class="btn btn-outline-info me-3">RESTART SEQUENCE</button>
                    <button onclick="renderMainMenu()" class="btn btn-info">MAIN MENU</button>
                </div>
            </div>`;
        transitionView(completeHtml);
        return;
    }
    state.currentChallenge = chal;
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
    <div class="row mb-5 pb-4 border-bottom" style="border-color: rgba(255,255,255,0.05) !important;">
        <div class="col-12">
            <button onclick="renderMainMenu()" class="btn btn-outline-info border-0 px-0">&larr; BACK</button>
        </div>
    </div>
    <div class="row justify-content-center">
        <div class="col-md-6 col-lg-5">
            <div class="glass-panel p-5">
                <h2 class="mb-4">PLAY AS GUEST</h2>
                <p class="code-font text-secondary mb-5">MODE: <span class="text-light">${escapeHTML(category).toUpperCase()}</span></p>
                <form onsubmit="launchGuestGame(event)">
                    <div class="mb-5">
                        <label class="form-label code-font text-secondary" style="font-size: 0.7rem;">GUEST NAME</label>
                        <input type="text" id="playerName" class="form-control" required autocomplete="off" maxlength="31">
                    </div>
                    <button type="submit" class="btn btn-info w-100">CONTINUE</button>
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
        <div class="d-flex justify-content-center align-items-center flex-grow-1">
            <p class="code-font text-secondary" style="letter-spacing: 0.2em;">LOADING CHALLENGE...</p>
        </div>`;

    try {
        const authRes = await fetch('/api/auth/guest', {
            method: 'POST',
            credentials: 'same-origin'
        });
    } catch (err) {}
    loadChallenges();
}

async function launchGameDirectly() {
    appDiv.innerHTML = `
        <div class="d-flex justify-content-center align-items-center flex-grow-1">
            <p class="code-font text-secondary" style="letter-spacing: 0.2em;">LOADING CHALLENGE...</p>
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
        state.challenges = MOCK_DB.challenges;
    }

    nextChallenge();
}

function renderChallenge(chal) {
    let html = `
    <div class="row mb-5 pb-4 border-bottom" style="border-color: rgba(255,255,255,0.05) !important;">
        <div class="col-12 d-flex justify-content-between align-items-center">
            <button onclick="renderMainMenu()" class="btn btn-outline-info border-0 px-0">&larr; EXIT</button>
            <span class="code-font text-secondary" style="font-size: 0.8rem;">ID: ${escapeHTML(chal.id)}</span>
        </div>
    </div>
    <div class="row justify-content-center">
        <div class="col-lg-8">
            <div class="mb-5">
                <span class="badge bg-dark mb-4">${escapeHTML(chal.difficulty)}</span>
                <h2 class="mb-3">${escapeHTML(chal.title)}</h2>
                <p class="text-secondary" style="font-size: 1.1rem;">${escapeHTML(chal.description)}</p>
            </div>`;

    if (chal.codeblock && chal.codeblock.length > 0) {
        if (chal.hex_fields && chal.hex_fields.length > 0) {
            let byteIdx = 0;
            const processedLines = chal.codeblock.map(line => {
                return line.split(' ').map(token => {
                    if (/^[0-9a-fA-F]{2}$/.test(token)) {
                        let span = `<span class="hex-byte" data-idx="${byteIdx}">${escapeHTML(token)}</span>`;
                        byteIdx++;
                        return span;
                    }
                    return escapeHTML(token);
                }).join(' ');
            });

            html += `
                <div class="mb-5">
                    <div class="d-flex justify-content-between align-items-center mb-2 px-1">
                        <span class="code-font text-secondary" style="font-size: 0.7rem; letter-spacing: 0.1em;">PAYLOAD EXAMINER</span>
                        <span id="hex-inspector" class="code-font hex-accent" style="font-size: 0.7rem; letter-spacing: 0.1em;">AWAITING HOVER</span>
                    </div>
                    <div class="hex-container-wrapper p-4" style="background: rgba(0,0,0,0.4); border: 1px solid rgba(255,255,255,0.05); overflow-x: auto;">
                        <pre class="mb-0"><code class="code-font text-light" id="hex-container">${processedLines.join('\n')}</code></pre>
                    </div>
                </div>`;
        } else {
            html += `
                <div class="p-4 mb-5" style="background: rgba(0,0,0,0.4); border: 1px solid rgba(255,255,255,0.05); overflow-x: auto;">
                    <pre class="mb-0"><code class="code-font text-light">${escapeHTML(chal.codeblock.join('\n'))}</code></pre>
                </div>`;
        }
    }

    html += `<form id="submitForm" onsubmit="submitAnswers(event)">`;

    chal.questions.forEach((q, index) => {
        html += `
            <div class="q-container">
                <div class="d-flex justify-content-between align-items-start mb-4">
                    <h5 class="mb-0 pe-4" style="font-family: 'Inter', sans-serif; font-weight: 400;">${escapeHTML(q.question)}</h5>
                    <span class="code-font hex-accent flex-shrink-0">${q.points} PTS</span>
                </div>`;

        if (q.type === 'text') {
            const placeholder = q.ui_hints?.placeholder || "ANSWER...";
            html += `<input type="text" name="q${q.id}" class="form-control w-100" placeholder="${escapeHTML(placeholder)}" autocomplete="off" required>`;
        } else if (q.type === 'multiple_choice') {
            html += `<div class="mt-3">`;
            q.responses.forEach(r => {
                html += `
                    <div class="form-check mb-3">
                        <input class="form-check-input" type="radio" name="q${q.id}" id="q${q.id}_r${r.id}" value="${r.id}" required>
                        <label class="form-check-label code-font text-light" for="q${q.id}_r${r.id}" style="font-size: 0.9rem;">${escapeHTML(r.text)}</label>
                    </div>`;
            });
            html += `</div>`;
        } else if (q.type === 'multiple_response') {
            html += `<div class="mt-3">`;
            q.responses.forEach(r => {
                html += `
                    <div class="form-check mb-3">
                        <input class="form-check-input" type="checkbox" data-question="q${q.id}" id="q${q.id}_r${r.id}" value="${r.id}">
                        <label class="form-check-label code-font text-light" for="q${q.id}_r${r.id}" style="font-size: 0.9rem;">${escapeHTML(r.text)}</label>
                    </div>`;
            });
            html += `</div>`;
        }
        html += `</div>`;
    });

    html += `
                <div class="pt-5 mt-3 border-top" style="border-color: rgba(255,255,255,0.05) !important;">
                    <button type="submit" class="btn btn-info w-100 py-3">SUBMIT</button>
                </div>
            </form>
        </div>
    </div>`;

    transitionView(html, () => {
        const container = document.getElementById('hex-container');
        const inspector = document.getElementById('hex-inspector');

        if (container && inspector) {
            container.addEventListener('mouseover', (e) => {
                if (e.target.classList.contains('hex-byte')) {
                    const idx = parseInt(e.target.getAttribute('data-idx'));
                    let fieldName = "UNKNOWN FIELD";
                    let start = idx;
                    let length = 1;

                    if (state.currentChallenge.hex_fields) {
                        const field = state.currentChallenge.hex_fields.find(f => idx >= f.start && idx < f.start + f.length);
                        if (field) {
                            fieldName = field.name;
                            start = field.start;
                            length = field.length;
                        }
                    }

                    const hexOffset = `0x${idx.toString(16).padStart(2, '0').toUpperCase()}`;
                    inspector.innerText = `[ OFFSET: ${hexOffset} ] ${fieldName}`;

                    document.querySelectorAll('.hex-byte.highlighted').forEach(el => el.classList.remove('highlighted'));

                    for (let i = start; i < start + length; i++) {
                        const byteEl = container.querySelector(`.hex-byte[data-idx="${i}"]`);
                        if (byteEl) byteEl.classList.add('highlighted');
                    }
                }
            });

            container.addEventListener('mouseout', (e) => {
                document.querySelectorAll('.hex-byte.highlighted').forEach(el => el.classList.remove('highlighted'));
                inspector.innerText = "AWAITING HOVER";
            });
        }
    });
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
        <div class="d-flex justify-content-center align-items-center flex-grow-1">
            <p class="code-font text-secondary" style="letter-spacing: 0.2em;">VERIFYING...</p>
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
                <div class="row justify-content-center align-items-center flex-grow-1 py-5">
                    <div class="col-md-6 text-center">
                        <h2 class="text-danger mb-4">TRANSMISSION FAILED</h2>
                        <p class="text-secondary mb-5">${escapeHTML(result.error) || "UNKNOWN ERROR."}</p>
                        <button onclick="renderMainMenu()" class="btn btn-outline-info">BACK</button>
                    </div>
                </div>`;
            transitionView(errorHtml);
            return;
        }

        if (result.total_earned === result.max_points) {
            const hId = hashStr(state.currentChallenge.id);
            if (!state.config.completedHashes.includes(hId)) {
                state.config.completedHashes.push(hId);
                if (!state.config.isGuest) {
                    state.config.completedCount++;
                }
            }
        }

        if (result.total_earned > 0 && !state.config.isGuest) {
            state.config.points += result.total_earned;
        }

        let headerText = "PARTIAL MATCH";
        let headerClass = "text-secondary";
        if (result.total_earned === result.max_points) {
            headerText = "SUCCESS";
            headerClass = "hex-accent";
        } else if (result.total_earned === 0) {
            headerText = "FAILED";
            headerClass = "text-danger";
        }

        let resultHtml = `
            <div class="row justify-content-center py-5">
                <div class="col-lg-8">
                    <div class="glass-panel p-5">
                        <h2 class="${headerClass} text-center mb-5">${headerText}</h2>

                        <div class="d-flex justify-content-center mb-5">
                            <div class="text-center">
                                <p class="code-font text-secondary mb-1" style="font-size: 0.7rem;">SCORE</p>
                                <h1 class="display-4 fw-bold mb-0">${result.total_earned} <span class="text-secondary fs-4">/ ${result.max_points}</span></h1>
                            </div>
                        </div>

                        <div class="mb-5">`;

        if (result.results && Array.isArray(result.results)) {
            result.results.forEach(r => {
                const isCorrect = r.correct === true || r.correct === "true";
                const blockClass = isCorrect ? "rgba(212, 175, 55, 0.05)" : "rgba(255, 0, 0, 0.05)";
                const borderClass = isCorrect ? "rgba(212, 175, 55, 0.2)" : "rgba(255, 0, 0, 0.2)";
                const textClass = isCorrect ? "hex-accent" : "text-danger";

                resultHtml += `
                    <div class="p-4 mb-4" style="background: ${blockClass}; border: 1px solid ${borderClass};">
                        <div class="d-flex justify-content-between align-items-center mb-3">
                            <span class="code-font ${textClass}">Q${r.id} // ${isCorrect ? 'CORRECT' : 'INCORRECT'}</span>
                            <span class="code-font text-light">${r.earned} PTS</span>
                        </div>
                        <p class="mb-0 text-secondary" style="font-size: 0.95rem;">
                            <span class="code-font text-light">EXPLANATION:</span> ${escapeHTML(r.explanation)}
                        </p>
                    </div>`;
            });
        }

        resultHtml += `
                        </div>
                        <div class="d-flex justify-content-center gap-4 border-top pt-5" style="border-color: rgba(255,255,255,0.05) !important;">
                            <button onclick="renderMainMenu()" class="btn btn-outline-info">MAIN MENU</button>
                            <button onclick="nextChallenge()" class="btn btn-info">CONTINUE</button>
                        </div>
                    </div>
                </div>
            </div>`;

        transitionView(resultHtml);

    } catch (err) {
        appDiv.innerHTML = `
            <div class="row justify-content-center align-items-center flex-grow-1 py-5">
                <div class="col-md-6 text-center">
                    <h2 class="text-danger mb-4">OFFLINE</h2>
                    <button onclick="renderMainMenu()" class="btn btn-outline-info">BACK</button>
                </div>
            </div>`;
    }
}

async function deleteAccount() {
    if (!confirm("Are you sure you'd like to delete your account?")) return;

    try {
        const res = await fetch('/api/user/delete', {
            method: 'POST',
            credentials: 'same-origin'
        });
        if (res.ok) {
            state.config.playerName = null;
            state.config.points = 0;
            state.config.isGuest = false;
            state.config.completedHashes = [];
            renderMainMenu();
        }
    } catch (err) {
        console.error("Deletion failed", err);
    }
}
