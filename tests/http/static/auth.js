const elements = {
    loginWrapper: document.getElementById('login-wrapper'),
    loginForm: document.getElementById('login-form'),
    loginText: document.getElementById('login-text')
};

function showError(message) {
    elements.loginText.style.display = "block";
    elements.loginText.textContent = message;
}

function isValidUserData(data) {
    return (
        data &&
        typeof data.id === 'number' &&
        typeof data.role === 'number' &&
        typeof data.name === 'string' &&
        typeof data.isClockedIn === 'number'
    );
}

function handleLoggedIn(data) {
    user.id = data.id;
    user.role = data.role;
    user.name = data.name;
    user.isClockedIn = data.isClockedIn;

    elements.loginWrapper.style.display = "none";
    elements.loginText.style.display = "none";

    userText.textContent = user.name;
    userPanel.style.display = "flex";
    menu.style.display = "block";
}

function handleLoggedOut() {
    elements.loginWrapper.style.display = "flex";
}

checkSession();

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
        if (response.status === 200) { // Ok
            const data = await response.json();
            if (isValidUserData(data)) {
                handleLoggedIn(data);
            } else {
                throw new Error('Sesión inválida');
            }
        } else if (response.status === 204) { // No Content
            throw new Error('Sesión inválida');
        } else if (response.status === 401) { // Unauthorized
            handleLoggedOut();
        } else {
            const data = await response.text();
            if (!data || data.trim() === '') {
                throw new Error('Sesión inválida');
            } else {
                throw new Error(data);
            }
        }
    } catch (error) {
        clearTimeout(timeoutId);
        showError(error.message);
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
            body: JSON.stringify({ email, password }),
            credentials: 'include'
        });

        if (response.status === 200) { // Ok
            checkSession();
        } else if (response.status === 204) { // No Content
            throw new Error('Sesión inválida, revise sus credenciales');
        } else {
            const data = await response.text();
            if (!data || data.trim() === '') {
                throw new Error('Sesión inválida, revise sus credenciales');
            } else {
                throw new Error(data);
            }
        }
    } catch (error) {
        showError(error.message);
    }
});

/*
// Login
elements.loginForm.addEventListener('submit', async (e) => {
    e.preventDefault();
    const username = document.getElementById('username').value;
    const password = document.getElementById('password').value;

    try {
        const response = await fetch('/login', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ username, password }),
            credentials: 'include'
        });

        if (response.ok) {
            checkSession();
        } else {
            alert("Credenciales incorrectas");
        }
    } catch (error) {
        console.error("Error:", error);
    }
});

document.getElementById('login-form').addEventListener('submit', function(e) {
    e.preventDefault();
    let isValid = true;
    if (isValid) {
        userPanel.style.display = "flex";
        menu.style.display = "block";
        loginWrapper.style.display = "none";
    }
});
*/

/*
// Fichar
elements.ficharBtn.addEventListener('click', async () => {
    const response = await fetch('/api/fichar', { 
        method: 'POST',
        credentials: 'include'
    });
    if (response.ok) {
        alert("Fichaje registrado");
    }
});

// Logout
elements.logoutBtn.addEventListener('click', async () => {
    await fetch('/api/logout', { 
        method: 'POST',
        credentials: 'include' 
    });
    handleLoggedOut();
});

// Helpers
function handleLoggedIn(username) {
    document.body.classList.remove('logged-out');
    document.body.classList.add('logged-in');
    elements.welcomeMessage.textContent = `Bienvenido, ${username}`;
}

function handleLoggedOut() {
    document.body.classList.remove('logged-in');
    document.body.classList.add('logged-out');
    elements.loginForm.reset();
}
*/

