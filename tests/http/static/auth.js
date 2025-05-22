const login = {
    frame: document.getElementById('login-frame'),
    form: document.getElementById('login-form'),
    text: document.getElementById('login-text')
};

function showLogin() {
    login.text.style.display = "none";
    login.frame.style.display = "flex";
}

function hideLogin() {
    hideLoginError();
    login.frame.style.display = "none";
    start();
}

function showLoginError(message) {
    if (login.text.style.display === "none") {
        login.text.style.display = "block";
    }
    login.text.textContent = message;
}

function hideLoginError() {
    if (login.text.style.display === "block") {
        login.text.style.display = "none";
    }
}

async function handleLogin(data) {
    try {
        user.id = data.id;
        user.role = data.role;
        user.name = data.name;
        user.clockInTime = await getClockInTime();
        hideLogin();
    } catch (error) {
        showLoginError(error.message || "Sesión inválida");
    }
}

login.form.addEventListener('submit', async (e) => {
    e.preventDefault();
    const email = document.getElementById('email').value;
    const password = document.getElementById('password').value;

    try {
        const response = await fetch('/api/login', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            credentials: 'include',
            body: JSON.stringify({ email, password })
        });

        if (response.status === 200) {
            await checkSession();
        } else {
            const data = await response.text();
            throw new Error(data || 'Sesión inválida, revise sus credenciales');
        }
    } catch (error) {
        showLoginError(error.message || "Sesión inválida");
    }
});

async function checkSession() {
    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), 5000);

    try {
        const response = await fetch('/api/auth', {
            method: 'GET',
            credentials: 'include',
            signal: controller.signal,
        });

        clearTimeout(timeoutId);
        if (response.status === 200) {
            const data = await response.json();
            await handleLogin(data);
        } else if (response.status === 401) {
            showLogin();
        } else {
            const data = await response.text();
            throw new Error(data);
        }
    } catch (error) {
        clearTimeout(timeoutId);
        showLoginError(error.message || 'Sesión inválida');
    } finally {
        controller.abort();
    }
}

checkSession();

