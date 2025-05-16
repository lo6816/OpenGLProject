# 3D Graphics Project – Interactive Island Scene

## 📦 Dependencies

Before building the project, make sure the following library is installed on your system:

- **[FreeType](https://freetype.org/)**: Required to render text and display characters in the scene.

On macOS (with Homebrew), you can install it using:

```bash
brew install freetype
```

## 🛠️ Build Instructions

This project uses **CMake** for configuration. To compile:

```bash
mkdir build
cd build
cmake ..
make
```

## ▶️ Running the Program

Once compiled, you can launch the main program with:

```bash
./infoh502-cpp_Project_main
```

## 🎮 Controls

The application offers first-person navigation in a 3D island environment.

### 🧭 Camera Movement

- `Z` – Move forward  
- `S` – Move backward  
- `Q` – Move left  
- `D` – Move right  
- `A` – Move up  
- `E` – Move down  
- `G` – Toggle **God Mode** (unrestricted free movement)
- `H` – Show/hide **hitboxes**
- `O` / `I` – Move the **sun** in the sky

---

Feel free to explore the island freely and test all the visual effects!
