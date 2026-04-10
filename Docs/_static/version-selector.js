// Version selector for Sphinx documentation on GitHub Pages
// Fetches GitHub tags and creates a dropdown in the navigation

document.addEventListener('DOMContentLoaded', function() {
    // Assuming the repo is una-watch/una-sdk, adjust if needed
    const repoOwner = 'una-watch';
    const repoName = 'una-sdk';
    const baseUrl = `https://${repoOwner}.github.io/${repoName}/`;

    // Create the selector element
    const selector = document.createElement('select');
    selector.id = 'version-selector';

    // Add loading option
    const loadingOption = document.createElement('option');
    loadingOption.textContent = 'Loading versions...';
    selector.appendChild(loadingOption);

    // Insert below the header title in the sidebar search area
    const searchArea = document.querySelector('.wy-side-nav-search');
    const searchContainer = searchArea ? searchArea.querySelector('[role="search"]') : null;
    if (searchArea && searchContainer) {
        searchArea.insertBefore(selector, searchContainer);
    } else if (searchArea) {
        searchArea.appendChild(selector);
    } else {
        const nav = document.querySelector('.wy-nav-side') || document.querySelector('nav') || document.body;
        nav.appendChild(selector);
    }

    // Fetch tags from GitHub API
    fetch(`https://api.github.com/repos/${repoOwner}/${repoName}/tags`)
        .then(response => response.json())
        .then(tags => {
            // Clear loading option
            selector.innerHTML = '';

            // Add current version option
            const currentOption = document.createElement('option');
            currentOption.textContent = 'latest';
            currentOption.value = baseUrl;
            selector.appendChild(currentOption);

            // Add tag options
            tags.forEach(tag => {
                const option = document.createElement('option');
                option.textContent = tag.name;
                option.value = `${baseUrl}${tag.name}/`;
                selector.appendChild(option);
            });

            // Set current version based on URL
            const currentPath = window.location.pathname;
            const versionMatch = currentPath.match(/\/([^\/]+)\//);
            if (versionMatch) {
                selector.value = `${baseUrl}${versionMatch[1]}/`;
            } else {
                selector.value = baseUrl;
            }

            // Handle change
            selector.addEventListener('change', function() {
                window.location.href = this.value;
            });
        })
        .catch(error => {
            console.error('Failed to load versions:', error);
            selector.innerHTML = '<option>Error loading versions</option>';
        });
});