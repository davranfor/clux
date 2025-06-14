const profile = {
    id: profileForm.querySelector('input[name="id"]'),
    workplace: profileForm.querySelector('select[name="workplace"]'),
    category: profileForm.querySelector('select[name="category"]'),
    role: profileForm.querySelector('select[name="role"]'),
    name: profileForm.querySelector('input[name="name"]'),
    tin: profileForm.querySelector('input[name="tin"]'),
    address: profileForm.querySelector('textarea[name="address"]'),
    phone: profileForm.querySelector('input[name="phone"]'),
    email: profileForm.querySelector('input[name="email"]')
}

async function profileEdit(id) {
    const response = await fetch(`/api/users/edit`, {
        method: 'GET',
        credentials: 'include'
    });

    if (response.status === 200) {
        const data = await response.json();
        profileEditUI(data);
    } else if (response.status === 204) {
        showMessage('El registro ya no existe');
    } else {
        const text = await response.text();
        throw new Error(text || `HTTP Error ${response.status}`);
    }
}

function profileEditUI(data) {
    profile.id.value = data.id;
    profile.workplace.replaceChildren();
    data.workplaces.forEach(workplace => {
        const option = document.createElement('option');

        option.value = workplace[0];
        option.textContent = workplace[1];
        profile.workplace.appendChild(option);
    });
    profile.workplace.value = data.workplace_id;
    profile.category.replaceChildren();
    data.categories.forEach(category => {
        const option = document.createElement('option');

        option.value = category[0];
        option.textContent = category[1];
        profile.category.appendChild(option);
    });
    profile.category.value = data.category_id;
    profile.role.value = data.role;
    profile.name.value = data.name;
    profile.tin.value = data.tin;
    profile.address.value = data.address;
    profile.phone.value = data.phone;
    profile.email.value = data.email;
}

profileForm.addEventListener('submit', (e) => {
    e.preventDefault();

    const elements = profileForm.elements;
    
    for (let i = 0; i < elements.length; i++) {
        const element = elements[i];
        
        if (element.tagName === 'INPUT' || element.tagName === 'TEXTAREA') {
            element.value = element.value.trim();
        }
    }

    const name = profile.name.value;
    const tin = profile.tin.value;
    const address = profile.address.value;
    const phone = profile.phone.value;
    const email = profile.email.value;

    if (user.role === role.ADMIN) {
        const workplace_id = Number(profile.workplace.value);
        const category_id = Number(profile.category.value);
        const role = Number(profile.role.value);
 
        profileUpdate(JSON.stringify({ workplace_id, category_id, role, name, tin, address, phone, email }));
    } else {
        profileUpdate(JSON.stringify({ name, tin, address, phone, email }));
    }

});

document.getElementById("profile-cancel").addEventListener("click", () => {
    profileEdit(user.id).catch(error => { if (error.message) showMessage(error.message); });
});

async function profileUpdate(data) {
    try {
        const response = await fetch(`/api/users/update`, {
            method: 'PUT',
            headers: { 'Content-Type': 'application/json' },
            credentials: 'include',
            body: data
        });

        if (response.status === 200) {
            await response.text();
            showMessage('Registro guardado');
        } else if (response.status === 204) {
            showMessage('No hay ningún cambio que guardar');
        } else {
            const text = await response.text();
            showMessage(text || `HTTP ${response.status}`);
        }
    } catch (error) {
        showMessage(error.message || 'No se puede actualizar el registro');
    }
}


