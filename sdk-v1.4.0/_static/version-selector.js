// Version selector for the Sphinx docs.
//
// Reads /versions.json (written at deploy time by .github/workflows/docs.yml),
// so the dropdown lists exactly the versions that are actually published — every
// entry links to a real page, never a 404. Docs are served at
// <site>/<version>/..., so the first path segment is the current version.

document.addEventListener('DOMContentLoaded', function () {
    const selector = document.createElement('select');
    selector.id = 'version-selector';
    selector.style.cssText = `
        display: block;
        margin: 8px auto 12px;
        padding: 5px 10px;
        font-size: 14px;
        border-radius: 4px;
        border: 1px solid rgba(255, 255, 255, 0.3);
        background: var(--white, #fff);
        color: #333;
        cursor: pointer;
    `;

    const loading = document.createElement('option');
    loading.textContent = 'Loading versions…';
    selector.appendChild(loading);

    // Insert below the header title in the RTD sidebar search area.
    const searchArea = document.querySelector('.wy-side-nav-search');
    const searchContainer = searchArea ? searchArea.querySelector('[role="search"]') : null;
    if (searchArea && searchContainer) {
        searchArea.insertBefore(selector, searchContainer);
    } else if (searchArea) {
        searchArea.appendChild(selector);
    } else {
        (document.querySelector('.wy-nav-side') || document.querySelector('nav') || document.body).appendChild(selector);
    }

    // Current version = first non-empty path segment (e.g. /sdk-v1.2.0/… -> sdk-v1.2.0).
    const segments = window.location.pathname.split('/').filter(Boolean);
    const current = segments.length ? segments[0] : '';

    // versions.json lives at the site root, regardless of custom domain.
    fetch('/versions.json', { cache: 'no-store' })
        .then(r => { if (!r.ok) throw new Error('versions.json ' + r.status); return r.json(); })
        .then(data => {
            const versions = Array.isArray(data.versions) ? data.versions : [];
            selector.innerHTML = '';
            versions.forEach(v => {
                const opt = document.createElement('option');
                opt.value = '/' + v + '/';
                opt.textContent = (v === data.latest) ? (v + ' (latest)') : v;
                if (v === current) opt.selected = true;
                selector.appendChild(opt);
            });
            if (!versions.length) {
                selector.innerHTML = '<option>No versions published</option>';
                return;
            }
            selector.addEventListener('change', function () {
                window.location.href = this.value;
            });
        })
        .catch(err => {
            console.error('version-selector: failed to load versions.json', err);
            selector.innerHTML = '<option>Versions unavailable</option>';
        });
});
