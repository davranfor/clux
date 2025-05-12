const elements = {
    userText: document.getElementById('user-text'),
    loginWrapper: document.getElementById('login-wrapper'),
    loginForm: document.getElementById('login-form')
/*
    fichajeSection: document.getElementById('fichajeSection'),
    welcomeMessage: document.getElementById('welcomeMessage'),
    ficharBtn: document.getElementById('ficharBtn'),
    logoutBtn: document.getElementById('logoutBtn')
*/
};

function showError(message) {
    const errorText = document.getElementById('error-text');
    errorText.style.display = "block";
    errorText.textContent = message;
}

// Verificar sesión al cargar
checkSession();

async function checkSession() {
    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), 5000); // Timeout 5 segundos

    try {
        const response = await fetch('/api/auth', {
            method: 'GET',
            credentials: 'include',   // Incluye cookies para autenticación
            signal: controller.signal // Para cancelar la petición
        });

        clearTimeout(timeoutId);

        // Caso 1: Sesión válida con datos (200 Ok + JSON)
        if (response.status === 200) {
            try {
                const data = await response.json();
                if (data?.name) {
                    handleLoggedIn(data.name);
                } else {
                    showError('Sesión invalida');
                }
            } catch (jsonError) {
                showError(`Error al parsear JSON: ${jsonError}`);
            }
        // Caso 2: Sin sesión (401 Unauthorized) (login)
        } else if (response.status === 401) {
            handleLoggedOut();
        // Caso 3: Sesión inválida, presumiblemente cookie manipulada
        } else {
            showError('Sesión inválida, por favor, contacte con el administrador.');
        }
    } catch (error) {
        clearTimeout(timeoutId);
        if (error.name === 'AbortError') {
            showError('Timeout al verificar sesión');
        } else if (error.name === 'TypeError') {
            showError(`Error de red o URL inválida: ${error.message}`);
        } else {
            showError(`Error inesperado: ${error}`);
        }
    }
}

// Login
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

        if (response.ok) {
            checkSession();
        } else {
            alert("Credenciales incorrectas");
        }
    } catch (error) {
        //console.error("Error:", error);
        alert(error);
    }
});

function handleLoggedIn(name) {
/*
    document.body.classList.remove('logged-out');
    document.body.classList.add('logged-in');
    elements.welcomeMessage.textContent = `Bienvenido, ${username}`;
*/
    elements.loginWrapper.style.display = "none";
    elements.userText.textContent = name;
    userPanel.style.display = "flex";
    menu.style.display = "block";
}

function handleLoggedOut() {
/*
    document.body.classList.remove('logged-in');
    document.body.classList.add('logged-out');
    elements.loginForm.reset();
*/
    elements.loginWrapper.style.display = "flex";
}

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

