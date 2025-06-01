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

login.form.addEventListener('submit', async (e) => {
    e.preventDefault();
    const email = login.form.querySelector('input[name="email"]').value;
    const password = login.form.querySelector('input[name="password"]').value;

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
    const timeout = setTimeout(() => controller.abort(), 5000);

    try {
        const response = await fetch('/api/auth', {
            method: 'GET',
            credentials: 'include',
            signal: controller.signal,
        });

        clearTimeout(timeout);
        if (response.status === 200) {
            [user.id, user.role, user.name] = await response.json();
            [user.workplace, user.clockIn] = await getClockIn();
            hideLogin();
        } else if (response.status === 401) {
            showLogin();
        } else {
            const data = await response.text();
            throw new Error(data);
        }
    } catch (error) {
        clearTimeout(timeout);
        showLoginError(error.message || 'Sesión inválida');
    } finally {
        controller.abort();
    }
}

checkSession();

