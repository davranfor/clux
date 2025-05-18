async function punch() {
    try {
        const response = await fetch('/api/timesheet', {
            method: 'POST',
            credentials: 'include'
        });

        if (response.status === 200) { // Ok
            const data = await response.json();
            if (data && typeof data === 'object') {
                user.clock_in = data[1] === '0000-00-00 00:00:00' ? data[0] : null;
            } else {
                throw new Error('Sesión inválida');
            }
        } else {
            const data = await response.text();
            if (!data || data.trim() === '') {
                throw new Error('Sesión inválida');
            } else {
                throw new Error(data);
            }
        }
    } catch (error) {
        alert(error.message);
    }
}

