import { Routes, Route } from "react-router-dom";
import { Sidebar } from "./components/Sidebar";
import { Header } from "./components/Header";
import Dashboard from "./pages/Dashboard";
import CPUBench from "./pages/CPUBench";
import GPUBench from "./pages/GPUBench";
import NPUBench from "./pages/NPUBench";
import Leaderboard from "./pages/Leaderboard";
import Settings from "./pages/Settings";

function App() {
    return (
        <div className="min-h-screen bg-background">
            <Sidebar />
            <Header />

            {/* Main Content */}
            <main className="ml-52 mt-14 p-6">
                <Routes>
                    <Route path="/" element={<Dashboard />} />
                    <Route path="/cpu" element={<CPUBench />} />
                    <Route path="/gpu" element={<GPUBench />} />
                    <Route path="/npu" element={<NPUBench />} />
                    <Route path="/leaderboard" element={<Leaderboard />} />
                    <Route path="/settings" element={<Settings />} />
                </Routes>
            </main>
        </div>
    );
}

export default App;
