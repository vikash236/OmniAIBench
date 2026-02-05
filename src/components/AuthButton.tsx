import { useState, useEffect } from "react";
import { User, onAuthStateChanged, signInWithPopup, signOut } from "firebase/auth";
import { auth, googleProvider } from "@/lib/firebase";
import { Button } from "@/components/ui/button";
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card";
import { LogIn, LogOut, User as UserIcon } from "lucide-react";

export function AuthButton() {
    const [user, setUser] = useState<User | null>(null);
    const [loading, setLoading] = useState(true);

    useEffect(() => {
        const unsubscribe = onAuthStateChanged(auth, (currentUser) => {
            setUser(currentUser);
            setLoading(false);
        });

        return () => unsubscribe();
    }, []);

    const handleSignIn = async () => {
        try {
            await signInWithPopup(auth, googleProvider);
        } catch (error) {
            console.error("Sign-in error:", error);
            alert("Failed to sign in. Please try again.");
        }
    };

    const handleSignOut = async () => {
        try {
            await signOut(auth);
        } catch (error) {
            console.error("Sign-out error:", error);
        }
    };

    if (loading) {
        return (
            <Button variant="ghost" disabled>
                <UserIcon className="mr-2 h-4 w-4" />
                Loading...
            </Button>
        );
    }

    if (user) {
        return (
            <div className="flex items-center gap-2">
                <div className="flex items-center gap-2 text-sm">
                    <img
                        src={user.photoURL || ""}
                        alt={user.displayName || "User"}
                        className="h-8 w-8 rounded-full border-2 border-primary"
                    />
                    <span className="text-foreground">{user.displayName}</span>
                </div>
                <Button variant="outline" size="sm" onClick={handleSignOut}>
                    <LogOut className="mr-2 h-4 w-4" />
                    Sign Out
                </Button>
            </div>
        );
    }

    return (
        <Button onClick={handleSignIn} size="sm">
            <LogIn className="mr-2 h-4 w-4" />
            Sign In with Google
        </Button>
    );
}

export function GuestModeCard() {
    return (
        <Card className="w-full max-w-md">
            <CardHeader>
                <CardTitle>Sign In</CardTitle>
                <CardDescription>
                    Sign in to save your benchmark results and compete on the global leaderboard
                </CardDescription>
            </CardHeader>
            <CardContent className="space-y-4">
                <AuthButton />
                <div className="text-center">
                    <p className="text-sm text-muted-foreground">
                        Or continue as guest (results won't be saved)
                    </p>
                </div>
            </CardContent>
        </Card>
    );
}
