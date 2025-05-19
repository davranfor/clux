function getClockIn() {
    return fetch('/api/timesheet', {
        method: 'GET',
        credentials: 'include'
    })
    .then(response => {
        return response.text().then(text => {
            if (response.ok) {
                return text || null;
            }
            throw new Error(text || `HTTP Error ${response.status}`);
        });
    });
}

async function punch() {
    try {
        const response = await fetch('/api/timesheet', {
            method: 'POST',
            credentials: 'include'
        });

        if (response.status === 200) {
            const data = await response.json();
            user.clock_in = data[1] === 0 ? data[0] : null;
        } else if (response.status === 204) {
            throw new Error('Deben pasar al menos 5 segundos entre fichajes');
        } else {
            const data = await response.text();
            throw new Error(data);
        }
    } catch (error) {
        alert(error.message || 'Sesión inválida');
    }
}

