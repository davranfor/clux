const login = {
    frame: document.getElementById('login-frame'),
    form: document.getElementById('login-form'),
    text: document.getElementById('login-text'),
    show() {
        this.text.style.display = "none";
        this.frame.style.display = "flex";
    },
    hide() {
        if (this.text.style.display === "block") {
            this.text.style.display = "none";
        }
        this.frame.style.display = "none";
    },
    showError(message) {
        if (this.text.style.display === "none") {
            this.text.style.display = "block";
        }
        this.text.textContent = message;
    }
};

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
            login.showError(data || 'Los datos introducidos no son correctos');
        }
    } catch (error) {
        login.showError(error.message || "Sesión inválida");
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
            [
                [user.role, user.name],
                [user.workplace, user.clockIn, clocking.elapsed]
            ] = await response.json();
            login.hide();
            start();
        } else if (response.status === 401) {
            login.show();
        } else {
            const data = await response.text();
            login.showError(data || 'Sesión inválida');
        }
    } catch (error) {
        clearTimeout(timeout);
        login.showError(error.message || 'Sesión inválida');
    } finally {
        controller.abort();
    }
}

checkSession();

