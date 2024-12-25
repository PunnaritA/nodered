const {Jimp} = require('jimp');

async function testJimp() {
    console.log('Jimp loaded:', typeof Jimp); // Should output 'function'
}

async function convertToRGB565(input) {
    try {
        let image;

        // If input is already a Jimp object, use it directly
        if (input.constructor.name === 'Jimp') {
            image = input;
        } else {
            // Otherwise, read the input as an image buffer or file path
            image = await Jimp.read(input);
        }

        // Resize the image
        const resizedImage = image.resize(120, 120);

        // Prepare for RGB565 conversion
        const width = resizedImage.bitmap.width;
        const height = resizedImage.bitmap.height;
        const rgb565Buffer = Buffer.alloc(width * height * 2); // 2 bytes per pixel

        let offset = 0;
        resizedImage.scan(0, 0, width, height, (x, y, idx) => {
            const r = resizedImage.bitmap.data[idx] >> 3; // Reduce to 5 bits
            const g = resizedImage.bitmap.data[idx + 1] >> 2; // Reduce to 6 bits
            const b = resizedImage.bitmap.data[idx + 2] >> 3; // Reduce to 5 bits

            const rgb565 = (r << 11) | (g << 5) | b; // Combine into RGB565
            rgb565Buffer.writeUInt16BE(rgb565, offset); // Big-endian format
            offset += 2;
        });

        // Return the RGB565 buffer
        return rgb565Buffer;
    } catch (err) {
        throw new Error(`Error processing image: ${err.message}`);
    }
}

// Export the function
module.exports = {
    testJimp,
    convertToRGB565,
};
