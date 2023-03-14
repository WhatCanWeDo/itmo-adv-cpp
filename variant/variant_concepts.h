#pragma once

template <typename T>
concept TriviallyDestructible = std::is_trivially_destructible_v<T>;

template <typename... Types>
concept AllTriviallyDestructible = ( std::is_trivially_destructible_v<Types> && ... );

template <typename... Types>
concept AllCopyConstructible = ( std::is_copy_constructible_v<Types> && ... );

template <typename... Types>
concept AllTriviallyCopyConstructible = ( std::is_trivially_copy_constructible_v<Types> && ... );

template <typename... Types>
concept AllCopyAssignable = ( std::is_copy_assignable_v<Types> && ... );

template <typename... Types>
concept AllTriviallyCopyAssignable = ( std::is_trivially_copy_assignable_v<Types> && ... );

template <typename... Types>
concept AllMoveConstructible = ( std::is_move_constructible_v<Types> && ... );

template <typename... Types>
concept AllTriviallyMoveConstructible = ( std::is_trivially_move_constructible_v<Types> && ... );

template <typename... Types>
concept AllMoveAssignable = ( std::is_move_assignable_v<Types> && ... );

template <typename... Types>
concept AllTriviallyMoveAssignable = ( std::is_trivially_move_assignable_v<Types> && ... );

template <typename... Types>
concept AllNothrowMoveConstructible = ( std::is_nothrow_move_constructible_v<Types> && ... );
