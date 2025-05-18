const elements = {
    loginWrapper: document.getElementById('login-wrapper'),
    loginForm: document.getElementById('login-form'),
    loginText: document.getElementById('login-text')
};

function updateUI() {
    elements.loginWrapper.style.display = "none";
    elements.loginText.style.display = "none";
    userText.textContent = user.name;
    userPanel.style.display = "flex";
    menu.style.display = "block";
}

function showError(message) {
    elements.loginText.style.display = "block";
    elements.loginText.textContent = message;
}

async function handleLoggedIn(data) {
    try {
        user.id = data.id;
        user.role = data.role;
        user.name = data.name;
        try {
            user.clock_in = await getClockIn();
        } catch (error) {
            throw new Error(error);
        }
        updateUserButton();
        updateUI();
    } catch (error) {
        showError(error.message || "Sesión inválida");
    }
}

function handleLoggedOut() {
    elements.loginWrapper.style.display = "flex";
}

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
            await handleLoggedIn(data);
        } else if (response.status === 401) {
            handleLoggedOut();
        } else {
            const data = await response.text();
            throw new Error(data);
        }
    } catch (error) {
        clearTimeout(timeoutId);
        showError(error.message || 'Sesión inválida');
    } finally {
        controller.abort();
    }
}

elements.loginForm.addEventListener('submit', async (e) => {
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
        showError(error.message || "Sesión inválida");
    }
});

checkSession();

