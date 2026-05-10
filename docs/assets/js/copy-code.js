// Nimble Documentation Copy Code

class CopyCodeManager {
    constructor() {
        this.codeBlocks = document.querySelectorAll('.code-block');
        this.init();
    }

    init() {
        this.codeBlocks.forEach(block => {
            this.addCopyButton(block);
        });
    }

    addCopyButton(block) {
        const button = document.createElement('button');
        button.className = 'copy-btn';
        button.textContent = 'Copy';
        button.addEventListener('click', () => this.copyCode(block, button));

        block.style.position = 'relative';
        block.appendChild(button);
    }

    async copyCode(block, button) {
        const code = block.querySelector('code') || block.querySelector('pre');
        const text = code ? code.textContent : block.textContent;

        try {
            await navigator.clipboard.writeText(text);
            button.textContent = 'Copied!';
            button.classList.add('copied');

            setTimeout(() => {
                button.textContent = 'Copy';
                button.classList.remove('copied');
            }, 2000);
        } catch (err) {
            console.error('Failed to copy code:', err);
            button.textContent = 'Failed';
            setTimeout(() => {
                button.textContent = 'Copy';
            }, 2000);
        }
    }
}

// Initialize copy functionality when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    new CopyCodeManager();
});