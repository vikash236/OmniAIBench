import { Link, useLocation } from "react-router-dom";
import { cn } from "@/lib/utils";
import {
    Home,
    Cpu,
    Gpu,
    Brain,
    Trophy,
    Settings,
} from "lucide-react";

const navigation = [
    { name: "Dashboard", href: "/", icon: Home },
    { name: "CPU Bench", href: "/cpu", icon: Cpu },
    { name: "GPU Bench", href: "/gpu", icon: Gpu },
    { name: "NPU Neural", href: "/npu", icon: Brain },
    { name: "Leaderboard", href: "/leaderboard", icon: Trophy },
    { name: "Settings", href: "/settings", icon: Settings },
];

export function Sidebar() {
    const location = useLocation();

    return (
        <div className="fixed inset-y-0 left-0 z-50 w-52 bg-[hsl(var(--sidebar-bg))] border-r border-border">
            {/* Logo - Clean and minimal */}
            <div className="flex h-12 items-center px-4 border-b border-border">
                <h1 className="text-sm font-semibold tracking-tight">
                    OmniAIBench
                </h1>
            </div>

            {/* Navigation - VS Code style */}
            <nav className="p-2 space-y-0.5">
                {navigation.map((item) => {
                    const isActive = location.pathname === item.href;
                    return (
                        <Link
                            key={item.name}
                            to={item.href}
                            className={cn(
                                "flex items-center gap-2 px-2 py-1.5 text-xs rounded-sm transition-colors",
                                isActive
                                    ? "bg-[hsl(var(--active-bg))] text-foreground"
                                    : "text-muted-foreground hover:bg-[hsl(var(--hover-bg))] hover:text-foreground"
                            )}
                        >
                            <item.icon className="h-4 w-4" />
                            {item.name}
                        </Link>
                    );
                })}
            </nav>

            {/* Environment Status - Minimal */}
            <div className="absolute bottom-0 left-0 right-0 border-t border-border p-2">
                <div className="flex items-center gap-1.5 px-2 py-1 text-xs text-muted-foreground">
                    <div className="h-1.5 w-1.5 rounded-full bg-green-500" />
                    <span>Ready</span>
                </div>
            </div>
        </div>
    );
}
